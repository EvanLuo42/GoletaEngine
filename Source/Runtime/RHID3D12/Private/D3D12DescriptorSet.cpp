/// @file
/// @brief Traditional descriptor-set binding. MVP covers CBV/SRV/UAV + samplers.

#include "D3D12DescriptorSet.h"

#include "D3D12Buffer.h"
#include "D3D12Device.h"
#include "D3D12FormatTable.h"
#include "D3D12Sampler.h"
#include "D3D12Texture.h"

namespace goleta::rhi::d3d12
{
namespace
{
bool isSamplerKind(RhiDescriptorKind K) noexcept
{
    return K == RhiDescriptorKind::Sampler;
}
} // namespace

Rc<D3D12DescriptorSetLayout> D3D12DescriptorSetLayout::create(const RhiDescriptorSetLayoutDesc& Desc) noexcept
{
    auto Self = Rc<D3D12DescriptorSetLayout>(new D3D12DescriptorSetLayout{});
    Self->Desc_ = Desc;
    if (Desc.Bindings && Desc.BindingCount > 0)
    {
        Self->Bindings.assign(Desc.Bindings, Desc.Bindings + Desc.BindingCount);
        Self->Desc_.Bindings = Self->Bindings.data();
        Self->Desc_.BindingCount = static_cast<uint32_t>(Self->Bindings.size());
    }

    // Two contiguous ranges: resource slots first, sampler slots second.
    Self->ResourceOffsets.resize(Self->Bindings.size(), 0xFFFFFFFFu);
    Self->SamplerOffsets.resize(Self->Bindings.size(), 0xFFFFFFFFu);
    for (size_t I = 0; I < Self->Bindings.size(); ++I)
    {
        const auto& B = Self->Bindings[I];
        if (isSamplerKind(B.Kind))
        {
            Self->SamplerOffsets[I] = Self->SamplerSlots;
            Self->SamplerSlots += B.ArraySize;
        }
        else
        {
            Self->ResourceOffsets[I] = Self->ResourceSlots;
            Self->ResourceSlots += B.ArraySize;
        }
    }
    if (Desc.DebugName)
        Self->Name = Desc.DebugName;
    return Self;
}

uint32_t D3D12DescriptorSetLayout::resourceSlotOffset(uint32_t Binding) const noexcept
{
    for (size_t I = 0; I < Bindings.size(); ++I)
        if (Bindings[I].Binding == Binding) return ResourceOffsets[I];
    return 0xFFFFFFFFu;
}

uint32_t D3D12DescriptorSetLayout::samplerSlotOffset(uint32_t Binding) const noexcept
{
    for (size_t I = 0; I < Bindings.size(); ++I)
        if (Bindings[I].Binding == Binding) return SamplerOffsets[I];
    return 0xFFFFFFFFu;
}

// ---------- D3D12DescriptorSet ----------

Rc<D3D12DescriptorSet> D3D12DescriptorSet::create(D3D12Device* Device, IRhiDescriptorSetLayout* Layout) noexcept
{
    if (!Device || !Layout)
        return {};

    auto Self   = Rc<D3D12DescriptorSet>(new D3D12DescriptorSet{});
    Self->OwnerDevice = Device;
    Self->Layout_     = Rc<IRhiDescriptorSetLayout>(Layout);

    auto* Lay = d3d12Cast<D3D12DescriptorSetLayout>(Layout);
    Self->ResourceCount = Lay->resourceSlotCount();
    Self->SamplerCount  = Lay->samplerSlotCount();

    auto& ResHeap = Device->bindless().resourceHeap();
    auto& SamHeap = Device->bindless().samplerHeap();

    if (Self->ResourceCount > 0)
    {
        Self->ResourceBase = ResHeap.allocateRangeFresh(Self->ResourceCount);
        if (Self->ResourceBase == InvalidBindlessIndex)
            return {};
    }
    if (Self->SamplerCount > 0)
    {
        Self->SamplerBase = SamHeap.allocateRangeFresh(Self->SamplerCount);
        if (Self->SamplerBase == InvalidBindlessIndex)
            return {};
    }
    return Self;
}

D3D12DescriptorSet::~D3D12DescriptorSet()
{
    // TODO(rhi): return the contiguous ranges to the heap. For MVP the heaps never reclaim
    //            range-allocated descriptor-set slots; with the 1M slot global heap this is a
    //            deliberate tradeoff traded against the root-table contiguity requirement.
    (void)OwnerDevice;
}

void D3D12DescriptorSet::writeConstantBuffer(uint32_t Binding, uint32_t ArrayIndex, IRhiBuffer* Buffer,
                                             uint64_t Offset, uint64_t Size)
{
    if (!OwnerDevice || !Layout_ || !Buffer)
        return;
    auto* Lay  = d3d12Cast<D3D12DescriptorSetLayout>(Layout_.get());
    auto* Buf  = d3d12Cast<D3D12Buffer>(Buffer);
    const uint32_t Off = Lay->resourceSlotOffset(Binding);
    if (Off == 0xFFFFFFFFu)
        return;
    const uint32_t Slot = ResourceBase + Off + ArrayIndex;

    D3D12_CONSTANT_BUFFER_VIEW_DESC V{};
    V.BufferLocation = Buf->gpuAddress() + Offset;
    V.SizeInBytes    = static_cast<UINT>((Size + 255ull) & ~255ull);
    OwnerDevice->raw()->CreateConstantBufferView(&V, OwnerDevice->bindless().resourceHeap().cpuHandle(Slot));
}

void D3D12DescriptorSet::writeSampledTexture(uint32_t Binding, uint32_t ArrayIndex, IRhiTextureView* View)
{
    if (!OwnerDevice || !Layout_ || !View)
        return;
    auto* Lay = d3d12Cast<D3D12DescriptorSetLayout>(Layout_.get());
    const uint32_t Off = Lay->resourceSlotOffset(Binding);
    if (Off == 0xFFFFFFFFu)
        return;
    const uint32_t Slot = ResourceBase + Off + ArrayIndex;

    auto* Vw = d3d12Cast<D3D12TextureView>(View);
    auto* Tex = d3d12Cast<D3D12Texture>(Vw->texture());
    if (!Tex)
        return;

    D3D12_SHADER_RESOURCE_VIEW_DESC V{};
    V.Format                  = toDxgiSrv(Vw->desc().Format == RhiFormat::Unknown ? Tex->desc().Format : Vw->desc().Format);
    V.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    V.ViewDimension           = D3D12_SRV_DIMENSION_TEXTURE2D;
    V.Texture2D.MostDetailedMip = Vw->desc().BaseMipLevel;
    V.Texture2D.MipLevels       = Vw->desc().MipLevelCount == 0 ? Tex->desc().MipLevels : Vw->desc().MipLevelCount;

    OwnerDevice->raw()->CreateShaderResourceView(Tex->raw(), &V, OwnerDevice->bindless().resourceHeap().cpuHandle(Slot));
}

void D3D12DescriptorSet::writeStorageTexture(uint32_t Binding, uint32_t ArrayIndex, IRhiTextureView* View)
{
    if (!OwnerDevice || !Layout_ || !View)
        return;
    auto* Lay = d3d12Cast<D3D12DescriptorSetLayout>(Layout_.get());
    const uint32_t Off = Lay->resourceSlotOffset(Binding);
    if (Off == 0xFFFFFFFFu)
        return;
    const uint32_t Slot = ResourceBase + Off + ArrayIndex;

    auto* Vw = d3d12Cast<D3D12TextureView>(View);
    auto* Tex = d3d12Cast<D3D12Texture>(Vw->texture());
    if (!Tex)
        return;

    D3D12_UNORDERED_ACCESS_VIEW_DESC V{};
    V.Format        = toDxgi(Vw->desc().Format == RhiFormat::Unknown ? Tex->desc().Format : Vw->desc().Format);
    V.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
    V.Texture2D.MipSlice = Vw->desc().BaseMipLevel;

    OwnerDevice->raw()->CreateUnorderedAccessView(Tex->raw(), nullptr, &V,
                                                  OwnerDevice->bindless().resourceHeap().cpuHandle(Slot));
}

void D3D12DescriptorSet::writeStorageBuffer(uint32_t Binding, uint32_t ArrayIndex, IRhiBuffer* Buffer,
                                            uint64_t Offset, uint64_t Size)
{
    if (!OwnerDevice || !Layout_ || !Buffer)
        return;
    auto* Lay = d3d12Cast<D3D12DescriptorSetLayout>(Layout_.get());
    const uint32_t Off = Lay->resourceSlotOffset(Binding);
    if (Off == 0xFFFFFFFFu)
        return;
    const uint32_t Slot = ResourceBase + Off + ArrayIndex;

    auto* Buf = d3d12Cast<D3D12Buffer>(Buffer);
    D3D12_UNORDERED_ACCESS_VIEW_DESC V{};
    V.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
    V.Format        = DXGI_FORMAT_UNKNOWN;
    if (Buf->desc().StructureStride > 0)
    {
        V.Buffer.FirstElement         = Offset / Buf->desc().StructureStride;
        V.Buffer.NumElements          = static_cast<UINT>(Size / Buf->desc().StructureStride);
        V.Buffer.StructureByteStride  = Buf->desc().StructureStride;
    }
    else
    {
        V.Format            = DXGI_FORMAT_R32_TYPELESS;
        V.Buffer.FirstElement = Offset / 4;
        V.Buffer.NumElements  = static_cast<UINT>(Size / 4);
        V.Buffer.Flags        = D3D12_BUFFER_UAV_FLAG_RAW;
    }
    OwnerDevice->raw()->CreateUnorderedAccessView(Buf->raw(), nullptr, &V,
                                                  OwnerDevice->bindless().resourceHeap().cpuHandle(Slot));
}

void D3D12DescriptorSet::writeSampler(uint32_t Binding, uint32_t ArrayIndex, IRhiSampler* Sampler)
{
    if (!OwnerDevice || !Layout_ || !Sampler)
        return;
    auto* Lay = d3d12Cast<D3D12DescriptorSetLayout>(Layout_.get());
    const uint32_t Off = Lay->samplerSlotOffset(Binding);
    if (Off == 0xFFFFFFFFu)
        return;
    const uint32_t Slot = SamplerBase + Off + ArrayIndex;

    // Re-create the sampler directly into the descriptor-set slot. Shader-visible heaps aren't
    // valid copy sources, so we can't CopyDescriptorsSimple from the sampler's own slot; using
    // the cached D3D12_SAMPLER_DESC is cheap and matches the semantics exactly.
    auto* S = d3d12Cast<D3D12Sampler>(Sampler);
    OwnerDevice->raw()->CreateSampler(&S->d3dDesc(), OwnerDevice->bindless().samplerHeap().cpuHandle(Slot));
}

} // namespace goleta::rhi::d3d12
