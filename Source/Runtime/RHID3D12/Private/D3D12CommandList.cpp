/// @file
/// @brief D3D12 command-list recording. Enhanced barriers + dynamic rendering path.

#include "D3D12CommandList.h"

#include <algorithm>

#include "D3D12BarrierTranslate.h"
#include "D3D12Buffer.h"
#include "D3D12DescriptorSet.h"
#include "D3D12Device.h"
#include "D3D12FormatTable.h"
#include "D3D12Pipeline.h"
#include "D3D12QueryHeap.h"
#include "D3D12Queue.h"
#include "D3D12Texture.h"

namespace goleta::rhi::d3d12
{
namespace
{

constexpr UINT kPixEventAnsiVersion = 1;

D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE toBeginType(const RhiLoadOp Op) noexcept
{
    switch (Op)
    {
    case RhiLoadOp::Load:
        return D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_PRESERVE;
    case RhiLoadOp::Clear:
        return D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_CLEAR;
    case RhiLoadOp::DontCare:
        return D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_DISCARD;
    }
    return D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_PRESERVE;
}

D3D12_RENDER_PASS_ENDING_ACCESS_TYPE toEndType(const RhiStoreOp Op) noexcept
{
    switch (Op)
    {
    case RhiStoreOp::Store:
        return D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE;
    case RhiStoreOp::DontCare:
        return D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_DISCARD;
    case RhiStoreOp::Resolve:
        return D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_RESOLVE;
    }
    return D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE;
}

} // namespace

Rc<D3D12CommandList> D3D12CommandList::create(D3D12Device* Device, D3D12CommandPool* Pool,
                                              ID3D12CommandAllocator* Allocator, const RhiQueueKind Kind) noexcept
{
    if (!Device || !Pool || !Allocator)
        return {};

    auto Self         = Rc<D3D12CommandList>(new D3D12CommandList{});
    Self->OwnerDevice = Device;
    Self->OwnerPool   = Pool;
    Self->Allocator   = ComPtr<ID3D12CommandAllocator>(Allocator);
    Self->Kind_       = Kind;

    const D3D12_COMMAND_LIST_TYPE     Type = toD3dCommandListType(Kind);
    ComPtr<ID3D12GraphicsCommandList> Base;
    const HRESULT Hr = Device->raw()->CreateCommandList(0, Type, Allocator, nullptr, IID_PPV_ARGS(&Base));
    if (FAILED(Hr))
    {
        GOLETA_LOG_ERROR(D3D12, "CreateCommandList: HRESULT 0x{:08x}", static_cast<unsigned>(Hr));
        return {};
    }
    Base->Close();
    if (FAILED(Base.As(&Self->List)))
    {
        GOLETA_LOG_ERROR(D3D12, "CommandList6/7 QueryInterface failed");
        return {};
    }
    return Self;
}

void D3D12CommandList::setDebugName(const char* NewName)
{
    Name = NewName ? NewName : "";
    if (List)
        setD3dObjectName(List.Get(), Name.c_str());
}

void D3D12CommandList::begin()
{
    if (Recording)
        return;
    List->Reset(Allocator.Get(), nullptr);
    HeapsBound           = false;
    GraphicsRootSigBound = false;
    ComputeRootSigBound  = false;
    Recording            = true;
}

void D3D12CommandList::end()
{
    if (!Recording)
        return;
    List->Close();
    Recording = false;
}

void D3D12CommandList::reset()
{
    Recording            = false;
    HeapsBound           = false;
    GraphicsRootSigBound = false;
    ComputeRootSigBound  = false;
}

void D3D12CommandList::bindDescriptorHeapsIfNeeded() noexcept
{
    if (HeapsBound || !OwnerDevice)
        return;
    ID3D12DescriptorHeap* Heaps[2] = {
        OwnerDevice->bindless().resourceHeap().heap(),
        OwnerDevice->bindless().samplerHeap().heap(),
    };
    List->SetDescriptorHeaps(2, Heaps);
    HeapsBound = true;
}

void D3D12CommandList::beginDebugScope(const char* NameIn, uint32_t /*ColorRgba*/)
{
    if (!NameIn)
        return;
    // Use BeginEvent with ANSI-version PIX encoding.
    const size_t Len = std::char_traits<char>::length(NameIn);
    List->BeginEvent(kPixEventAnsiVersion, NameIn, static_cast<UINT>(Len + 1));
}

void D3D12CommandList::endDebugScope() { List->EndEvent(); }

void D3D12CommandList::insertDebugMarker(const char* NameIn, uint32_t /*ColorRgba*/)
{
    if (!NameIn)
        return;
    const size_t Len = std::char_traits<char>::length(NameIn);
    List->SetMarker(kPixEventAnsiVersion, NameIn, static_cast<UINT>(Len + 1));
}

void D3D12CommandList::barriers(const RhiBarrierGroup& Group)
{
    std::vector<D3D12_GLOBAL_BARRIER>  Globals;
    std::vector<D3D12_BUFFER_BARRIER>  Buffers;
    std::vector<D3D12_TEXTURE_BARRIER> Textures;

    Globals.reserve(Group.GlobalCount);
    Buffers.reserve(Group.BufferCount);
    Textures.reserve(Group.TextureCount);

    for (uint32_t I = 0; I < Group.GlobalCount; ++I)
    {
        const auto& G = Group.Globals[I];
        Globals.push_back(
            {toD3dSync(G.SrcStages), toD3dSync(G.DstStages), toD3dAccess(G.SrcAccess), toD3dAccess(G.DstAccess)});
    }
    for (uint32_t I = 0; I < Group.BufferCount; ++I)
    {
        const auto& B   = Group.Buffers[I];
        auto*       Buf = d3d12Cast<D3D12Buffer>(B.Buffer);
        Buffers.push_back({toD3dSync(B.SrcStages), toD3dSync(B.DstStages), toD3dAccess(B.SrcAccess),
                           toD3dAccess(B.DstAccess), Buf ? Buf->raw() : nullptr, B.Offset, B.Size});
    }
    for (uint32_t I = 0; I < Group.TextureCount; ++I)
    {
        const auto&           T   = Group.Textures[I];
        auto*                 Tex = d3d12Cast<D3D12Texture>(T.Texture);
        D3D12_TEXTURE_BARRIER D{};
        D.SyncBefore                        = toD3dSync(T.SrcStages);
        D.SyncAfter                         = toD3dSync(T.DstStages);
        D.AccessBefore                      = toD3dAccess(T.SrcAccess);
        D.AccessAfter                       = toD3dAccess(T.DstAccess);
        D.LayoutBefore                      = toD3dLayout(T.SrcLayout);
        D.LayoutAfter                       = toD3dLayout(T.DstLayout);
        D.pResource                         = Tex ? Tex->raw() : nullptr;
        D.Subresources.IndexOrFirstMipLevel = T.BaseMipLevel;
        D.Subresources.NumMipLevels         = T.MipLevelCount == 0 ? 0xFFFFFFFFu : T.MipLevelCount;
        D.Subresources.FirstArraySlice      = T.BaseArrayLayer;
        D.Subresources.NumArraySlices       = T.ArrayLayerCount == 0 ? 0xFFFFFFFFu : T.ArrayLayerCount;
        D.Subresources.FirstPlane           = 0;
        D.Subresources.NumPlanes            = 1;
        D.Flags = T.DiscardContents ? D3D12_TEXTURE_BARRIER_FLAG_DISCARD : D3D12_TEXTURE_BARRIER_FLAG_NONE;
        Textures.push_back(D);
    }

    D3D12_BARRIER_GROUP Groups[3]{};
    uint32_t            NumGroups = 0;
    if (!Globals.empty())
    {
        auto& G           = Groups[NumGroups++];
        G.Type            = D3D12_BARRIER_TYPE_GLOBAL;
        G.NumBarriers     = static_cast<UINT32>(Globals.size());
        G.pGlobalBarriers = Globals.data();
    }
    if (!Buffers.empty())
    {
        auto& G           = Groups[NumGroups++];
        G.Type            = D3D12_BARRIER_TYPE_BUFFER;
        G.NumBarriers     = static_cast<UINT32>(Buffers.size());
        G.pBufferBarriers = Buffers.data();
    }
    if (!Textures.empty())
    {
        auto& G            = Groups[NumGroups++];
        G.Type             = D3D12_BARRIER_TYPE_TEXTURE;
        G.NumBarriers      = static_cast<UINT32>(Textures.size());
        G.pTextureBarriers = Textures.data();
    }
    if (NumGroups > 0)
        List->Barrier(NumGroups, Groups);
}

void D3D12CommandList::beginRendering(const RhiRenderingDesc& Desc)
{
    constexpr uint32_t                   kMaxRt = 8;
    D3D12_RENDER_PASS_RENDER_TARGET_DESC Rts[kMaxRt]{};
    D3D12_RENDER_PASS_DEPTH_STENCIL_DESC DsDesc{};
    const uint32_t RtCount = Desc.ColorAttachmentCount > kMaxRt ? kMaxRt : Desc.ColorAttachmentCount;

    for (uint32_t I = 0; I < RtCount; ++I)
    {
        const auto& A = Desc.ColorAttachments[I];
        if (!A.View)
            continue;
        auto*          View         = d3d12Cast<D3D12TextureView>(A.View);
        auto*          Tex          = d3d12Cast<D3D12Texture>(View->texture());
        const uint32_t RtvIdx       = Tex->ensureRtvIndex(View->desc().BaseMipLevel, View->desc().BaseArrayLayer);
        Rts[I].cpuDescriptor        = OwnerDevice->rtvHeap().cpuHandle(RtvIdx);
        Rts[I].BeginningAccess.Type = toBeginType(A.LoadOp);
        Rts[I].BeginningAccess.Clear.ClearValue.Format   = toDxgi(Tex->desc().Format);
        Rts[I].BeginningAccess.Clear.ClearValue.Color[0] = A.ClearColor[0];
        Rts[I].BeginningAccess.Clear.ClearValue.Color[1] = A.ClearColor[1];
        Rts[I].BeginningAccess.Clear.ClearValue.Color[2] = A.ClearColor[2];
        Rts[I].BeginningAccess.Clear.ClearValue.Color[3] = A.ClearColor[3];
        Rts[I].EndingAccess.Type                         = toEndType(A.StoreOp);
    }

    D3D12_RENDER_PASS_DEPTH_STENCIL_DESC* DsPtr = nullptr;
    if (Desc.DepthAttachment && Desc.DepthAttachment->View)
    {
        auto*          View              = d3d12Cast<D3D12TextureView>(Desc.DepthAttachment->View);
        auto*          Tex               = d3d12Cast<D3D12Texture>(View->texture());
        const uint32_t DsvIdx            = Tex->ensureDsvIndex(View->desc().BaseMipLevel, View->desc().BaseArrayLayer);
        DsDesc.cpuDescriptor             = OwnerDevice->dsvHeap().cpuHandle(DsvIdx);
        DsDesc.DepthBeginningAccess.Type = toBeginType(Desc.DepthAttachment->LoadOp);
        DsDesc.DepthBeginningAccess.Clear.ClearValue.Format             = toDxgiDsv(Tex->desc().Format);
        DsDesc.DepthBeginningAccess.Clear.ClearValue.DepthStencil.Depth = Desc.DepthAttachment->ClearDepth;
        DsDesc.DepthBeginningAccess.Clear.ClearValue.DepthStencil.Stencil =
            static_cast<UINT8>(Desc.DepthAttachment->ClearStencil);
        DsDesc.DepthEndingAccess.Type      = toEndType(Desc.DepthAttachment->StoreOp);
        DsDesc.StencilBeginningAccess.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_NO_ACCESS;
        DsDesc.StencilEndingAccess.Type    = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_NO_ACCESS;
        DsPtr                              = &DsDesc;
    }

    bindDescriptorHeapsIfNeeded();
    List->BeginRenderPass(RtCount, Rts, DsPtr, D3D12_RENDER_PASS_FLAG_NONE);
    InRendering = true;

    if (Desc.RenderArea.Width > 0 && Desc.RenderArea.Height > 0)
    {
        D3D12_VIEWPORT V{};
        V.TopLeftX = static_cast<float>(Desc.RenderArea.X);
        V.TopLeftY = static_cast<float>(Desc.RenderArea.Y);
        V.Width    = static_cast<float>(Desc.RenderArea.Width);
        V.Height   = static_cast<float>(Desc.RenderArea.Height);
        V.MinDepth = 0.0f;
        V.MaxDepth = 1.0f;
        const D3D12_RECT R{.left   = Desc.RenderArea.X,
                           .top    = Desc.RenderArea.Y,
                           .right  = static_cast<LONG>(Desc.RenderArea.X + Desc.RenderArea.Width),
                           .bottom = static_cast<LONG>(Desc.RenderArea.Y + Desc.RenderArea.Height)};
        List->RSSetViewports(1, &V);
        List->RSSetScissorRects(1, &R);
    }
}

void D3D12CommandList::endRendering()
{
    if (InRendering)
    {
        List->EndRenderPass();
        InRendering = false;
    }
}

void D3D12CommandList::setViewports(const RhiViewport* Viewports, uint32_t Count)
{
    constexpr uint32_t kMax = D3D12_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
    D3D12_VIEWPORT     V[kMax];
    Count = std::min(Count, kMax);
    for (uint32_t I = 0; I < Count; ++I)
    {
        V[I].TopLeftX = Viewports[I].X;
        V[I].TopLeftY = Viewports[I].Y;
        V[I].Width    = Viewports[I].Width;
        V[I].Height   = Viewports[I].Height;
        V[I].MinDepth = Viewports[I].MinDepth;
        V[I].MaxDepth = Viewports[I].MaxDepth;
    }
    List->RSSetViewports(Count, V);
}

void D3D12CommandList::setScissors(const RhiRect* Scissors, uint32_t Count)
{
    constexpr uint32_t kMax = D3D12_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
    D3D12_RECT         R[kMax];
    if (Count > kMax)
        Count = kMax;
    for (uint32_t I = 0; I < Count; ++I)
        R[I] = {Scissors[I].X, Scissors[I].Y, static_cast<LONG>(Scissors[I].X + Scissors[I].Width),
                static_cast<LONG>(Scissors[I].Y + Scissors[I].Height)};
    List->RSSetScissorRects(Count, R);
}

void D3D12CommandList::setBlendConstants(const float Rgba[4]) { List->OMSetBlendFactor(Rgba); }

void D3D12CommandList::setStencilReference(uint32_t Reference) { List->OMSetStencilRef(Reference); }

void D3D12CommandList::setGraphicsPipeline(IRhiGraphicsPipeline* Pipeline)
{
    if (!Pipeline)
        return;
    auto* P = d3d12Cast<D3D12GraphicsPipeline>(Pipeline);
    bindDescriptorHeapsIfNeeded();
    List->SetPipelineState(P->raw());
    List->SetGraphicsRootSignature(P->rootSignature());
    List->IASetPrimitiveTopology(P->primitiveTopology());
    GraphicsRootSigBound = true;
}

void D3D12CommandList::setComputePipeline(IRhiComputePipeline* Pipeline)
{
    if (!Pipeline)
        return;
    auto* P = d3d12Cast<D3D12ComputePipeline>(Pipeline);
    bindDescriptorHeapsIfNeeded();
    List->SetPipelineState(P->raw());
    List->SetComputeRootSignature(P->rootSignature());
    ComputeRootSigBound = true;
}

void D3D12CommandList::setRootConstants(const void* Data, uint32_t SizeBytes, uint32_t OffsetBytes)
{
    if (!Data || SizeBytes == 0)
        return;
    const UINT Num = SizeBytes / 4;
    const UINT Off = OffsetBytes / 4;
    if (GraphicsRootSigBound)
        List->SetGraphicsRoot32BitConstants(0, Num, Data, Off);
    if (ComputeRootSigBound)
        List->SetComputeRoot32BitConstants(0, Num, Data, Off);
}

void D3D12CommandList::setDescriptorSet(uint32_t /*SetIndex*/, IRhiDescriptorSet* /*Set*/)
{
    // TODO(rhi): implement descriptor-set root table bind once Pipeline builds the table params.
    // For MVP the bindless path covers all callers.
}

void D3D12CommandList::setVertexBuffers(uint32_t FirstBinding, IRhiBuffer* const* Buffers, const uint64_t* Offsets,
                                        uint32_t Count)
{
    constexpr uint32_t       kMax = D3D12_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT;
    D3D12_VERTEX_BUFFER_VIEW V[kMax];
    Count = std::min(Count, kMax);
    for (uint32_t I = 0; I < Count; ++I)
    {
        const auto* B       = d3d12Cast<D3D12Buffer>(Buffers[I]);
        V[I].BufferLocation = B ? B->gpuAddress() + (Offsets ? Offsets[I] : 0) : 0;
        V[I].SizeInBytes    = static_cast<UINT>(B ? B->desc().SizeBytes - (Offsets ? Offsets[I] : 0) : 0);
        V[I].StrideInBytes  = B ? B->desc().StructureStride : 0;
    }
    List->IASetVertexBuffers(FirstBinding, Count, V);
}

void D3D12CommandList::setIndexBuffer(IRhiBuffer* Buffer, const uint64_t Offset, const RhiIndexType Type)
{
    auto*                   B = d3d12Cast<D3D12Buffer>(Buffer);
    D3D12_INDEX_BUFFER_VIEW V{};
    V.BufferLocation = B ? B->gpuAddress() + Offset : 0;
    V.SizeInBytes    = static_cast<UINT>(B ? B->desc().SizeBytes - Offset : 0);
    V.Format         = Type == RhiIndexType::Uint16 ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;
    List->IASetIndexBuffer(&V);
}

void D3D12CommandList::draw(uint32_t VertexCount, uint32_t InstanceCount, uint32_t FirstVertex, uint32_t FirstInstance)
{
    List->DrawInstanced(VertexCount, InstanceCount, FirstVertex, FirstInstance);
}

void D3D12CommandList::drawIndexed(uint32_t IndexCount, uint32_t InstanceCount, uint32_t FirstIndex,
                                   int32_t VertexOffset, uint32_t FirstInstance)
{
    List->DrawIndexedInstanced(IndexCount, InstanceCount, FirstIndex, VertexOffset, FirstInstance);
}

void D3D12CommandList::drawIndirect(IRhiBuffer* /*Args*/, uint64_t /*Offset*/, uint32_t /*DrawCount*/,
                                    uint32_t /*Stride*/)
{
    // TODO(rhi): implement ExecuteIndirect with a pre-created CommandSignature.
}

void D3D12CommandList::drawIndexedIndirect(IRhiBuffer* /*Args*/, uint64_t /*Offset*/, uint32_t /*DrawCount*/,
                                           uint32_t /*Stride*/)
{
    // TODO(rhi): implement indirect indexed draws via CommandSignature.
}

void D3D12CommandList::dispatch(uint32_t X, uint32_t Y, uint32_t Z) { List->Dispatch(X, Y, Z); }

void D3D12CommandList::dispatchIndirect(IRhiBuffer* /*Args*/, uint64_t /*Offset*/)
{
    // TODO(rhi): implement ExecuteIndirect dispatch.
}

void D3D12CommandList::copyBuffer(IRhiBuffer* Dst, uint64_t DstOffset, IRhiBuffer* Src, uint64_t SrcOffset,
                                  uint64_t Size)
{
    auto* D = d3d12Cast<D3D12Buffer>(Dst);
    auto* S = d3d12Cast<D3D12Buffer>(Src);
    if (!D || !S)
        return;
    List->CopyBufferRegion(D->raw(), DstOffset, S->raw(), SrcOffset, Size);
}

void D3D12CommandList::copyTexture(const RhiTextureCopy& Copy)
{
    auto* Dst = d3d12Cast<D3D12Texture>(Copy.DstTexture);
    auto* Src = d3d12Cast<D3D12Texture>(Copy.SrcTexture);
    if (!Dst || !Src)
        return;
    D3D12_TEXTURE_COPY_LOCATION D{}, S{};
    D.pResource        = Dst->raw();
    D.Type             = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    D.SubresourceIndex = Copy.DstMipLevel + Copy.DstArrayLayer * Dst->desc().MipLevels;
    S.pResource        = Src->raw();
    S.Type             = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    S.SubresourceIndex = Copy.SrcMipLevel + Copy.SrcArrayLayer * Src->desc().MipLevels;
    D3D12_BOX Box{static_cast<UINT>(Copy.SrcX),
                  static_cast<UINT>(Copy.SrcY),
                  static_cast<UINT>(Copy.SrcZ),
                  static_cast<UINT>(Copy.SrcX + Copy.Width),
                  static_cast<UINT>(Copy.SrcY + Copy.Height),
                  static_cast<UINT>(Copy.SrcZ + Copy.Depth)};
    List->CopyTextureRegion(&D, Copy.DstX, Copy.DstY, Copy.DstZ, &S, &Box);
}

void D3D12CommandList::copyBufferToTexture(IRhiBuffer* Src, uint64_t SrcOffset, uint32_t RowPitch,
                                           const RhiTextureCopy& Dst)
{
    auto* S = d3d12Cast<D3D12Buffer>(Src);
    auto* D = d3d12Cast<D3D12Texture>(Dst.DstTexture);
    if (!S || !D)
        return;
    D3D12_TEXTURE_COPY_LOCATION DstLoc{}, SrcLoc{};
    DstLoc.pResource                          = D->raw();
    DstLoc.Type                               = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    DstLoc.SubresourceIndex                   = Dst.DstMipLevel + Dst.DstArrayLayer * D->desc().MipLevels;
    SrcLoc.pResource                          = S->raw();
    SrcLoc.Type                               = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    SrcLoc.PlacedFootprint.Offset             = SrcOffset;
    SrcLoc.PlacedFootprint.Footprint.Format   = toDxgi(D->desc().Format);
    SrcLoc.PlacedFootprint.Footprint.Width    = Dst.Width;
    SrcLoc.PlacedFootprint.Footprint.Height   = Dst.Height;
    SrcLoc.PlacedFootprint.Footprint.Depth    = Dst.Depth;
    SrcLoc.PlacedFootprint.Footprint.RowPitch = RowPitch;
    List->CopyTextureRegion(&DstLoc, Dst.DstX, Dst.DstY, Dst.DstZ, &SrcLoc, nullptr);
}

void D3D12CommandList::writeTimestamp(IRhiQueryHeap* Heap, uint32_t Index)
{
    auto* H = d3d12Cast<D3D12QueryHeap>(Heap);
    if (!H)
        return;
    List->EndQuery(H->raw(), H->d3dType(), Index);
}

void D3D12CommandList::resolveQueries(IRhiQueryHeap* Heap, uint32_t FirstQuery, uint32_t Count, IRhiBuffer* Dst,
                                      uint64_t DstOffset)
{
    auto* H = d3d12Cast<D3D12QueryHeap>(Heap);
    auto* D = d3d12Cast<D3D12Buffer>(Dst);
    if (!H || !D)
        return;
    List->ResolveQueryData(H->raw(), H->d3dType(), FirstQuery, Count, D->raw(), DstOffset);
}

bool D3D12CommandList::dispatchRays(const RhiDispatchRaysDesc&) { return false; }
bool D3D12CommandList::drawMeshTasks(uint32_t, uint32_t, uint32_t) { return false; }
bool D3D12CommandList::drawMeshTasksIndirect(IRhiBuffer*, uint64_t, uint32_t, uint32_t) { return false; }
bool D3D12CommandList::buildAccelStructure(const RhiAccelBuildDesc&) { return false; }

// ---------- D3D12CommandPool ----------

Rc<D3D12CommandPool> D3D12CommandPool::create(D3D12Device* Device, RhiQueueKind Kind) noexcept
{
    if (!Device)
        return {};
    auto Self         = Rc<D3D12CommandPool>(new D3D12CommandPool{});
    Self->OwnerDevice = Device;
    Self->Kind_       = Kind;
    const HRESULT Hr =
        Device->raw()->CreateCommandAllocator(toD3dCommandListType(Kind), IID_PPV_ARGS(&Self->Allocator));
    if (FAILED(Hr))
    {
        GOLETA_LOG_ERROR(D3D12, "CreateCommandAllocator: HRESULT 0x{:08x}", static_cast<unsigned>(Hr));
        return {};
    }
    return Self;
}

void D3D12CommandPool::setDebugName(const char* NewName)
{
    Name = NewName ? NewName : "";
    if (Allocator)
        setD3dObjectName(Allocator.Get(), Name.c_str());
}

Rc<IRhiCommandList> D3D12CommandPool::allocate()
{
    auto Cl = D3D12CommandList::create(OwnerDevice, this, Allocator.Get(), Kind_);
    if (Cl)
        Lists.push_back(Cl);
    return Cl;
}

void D3D12CommandPool::reset()
{
    if (Allocator)
        Allocator->Reset();
    for (auto& Cl : Lists)
        Cl->reset();
}

} // namespace goleta::rhi::d3d12
