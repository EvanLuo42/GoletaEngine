/// @file
/// @brief ID3D12Resource textures + per-view descriptor allocation.

#include "D3D12Texture.h"

#include "D3D12Device.h"
#include "D3D12FormatTable.h"

namespace goleta::rhi::d3d12
{
namespace
{

D3D12_RESOURCE_DIMENSION toDim(RhiTextureDimension D) noexcept
{
    switch (D)
    {
    case RhiTextureDimension::Tex1D:   return D3D12_RESOURCE_DIMENSION_TEXTURE1D;
    case RhiTextureDimension::Tex2D:
    case RhiTextureDimension::TexCube: return D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    case RhiTextureDimension::Tex3D:   return D3D12_RESOURCE_DIMENSION_TEXTURE3D;
    }
    return D3D12_RESOURCE_DIMENSION_TEXTURE2D;
}

D3D12_RESOURCE_FLAGS toFlags(RhiTextureUsage Usage) noexcept
{
    D3D12_RESOURCE_FLAGS F = D3D12_RESOURCE_FLAG_NONE;
    if (anyOf(Usage, RhiTextureUsage::Storage))          F |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    if (anyOf(Usage, RhiTextureUsage::ColorAttachment))  F |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    if (anyOf(Usage, RhiTextureUsage::DepthAttachment))  F |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
    return F;
}

D3D12_HEAP_TYPE toHeap(RhiMemoryLocation L) noexcept
{
    switch (L)
    {
    case RhiMemoryLocation::DeviceLocal: return D3D12_HEAP_TYPE_DEFAULT;
    case RhiMemoryLocation::Upload:      return D3D12_HEAP_TYPE_UPLOAD;
    case RhiMemoryLocation::Readback:    return D3D12_HEAP_TYPE_READBACK;
    }
    return D3D12_HEAP_TYPE_DEFAULT;
}

D3D12_RESOURCE_DESC makeTexDesc(const RhiTextureDesc& T) noexcept
{
    D3D12_RESOURCE_DESC D{};
    D.Dimension          = toDim(T.Dimension);
    D.Width              = T.Width;
    D.Height             = T.Height;
    D.DepthOrArraySize   = static_cast<UINT16>(T.DepthOrArrayLayers);
    D.MipLevels          = static_cast<UINT16>(T.MipLevels);
    D.SampleDesc.Count   = static_cast<UINT>(T.Samples);
    D.SampleDesc.Quality = 0;
    D.Format             = toDxgiTypeless(T.Format);
    D.Layout             = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    D.Flags              = toFlags(T.Usage);
    return D;
}

bool pickClearValue(const RhiTextureDesc& T, D3D12_CLEAR_VALUE& Out) noexcept
{
    if (anyOf(T.Usage, RhiTextureUsage::ColorAttachment) && T.OptimizedClearValue.UseColor)
    {
        Out.Format   = toDxgi(T.Format);
        Out.Color[0] = T.OptimizedClearValue.Color[0];
        Out.Color[1] = T.OptimizedClearValue.Color[1];
        Out.Color[2] = T.OptimizedClearValue.Color[2];
        Out.Color[3] = T.OptimizedClearValue.Color[3];
        return true;
    }
    if (anyOf(T.Usage, RhiTextureUsage::DepthAttachment) && !T.OptimizedClearValue.UseColor)
    {
        Out.Format               = toDxgiDsv(T.Format);
        Out.DepthStencil.Depth   = T.OptimizedClearValue.Depth;
        Out.DepthStencil.Stencil = static_cast<UINT8>(T.OptimizedClearValue.Stencil);
        return true;
    }
    return false;
}

D3D12_SRV_DIMENSION toSrvDim(RhiTextureDimension D, uint32_t ArrayLayers) noexcept
{
    const bool Array = ArrayLayers > 1;
    switch (D)
    {
    case RhiTextureDimension::Tex1D:   return Array ? D3D12_SRV_DIMENSION_TEXTURE1DARRAY   : D3D12_SRV_DIMENSION_TEXTURE1D;
    case RhiTextureDimension::Tex2D:   return Array ? D3D12_SRV_DIMENSION_TEXTURE2DARRAY   : D3D12_SRV_DIMENSION_TEXTURE2D;
    case RhiTextureDimension::Tex3D:   return D3D12_SRV_DIMENSION_TEXTURE3D;
    case RhiTextureDimension::TexCube: return Array ? D3D12_SRV_DIMENSION_TEXTURECUBEARRAY : D3D12_SRV_DIMENSION_TEXTURECUBE;
    }
    return D3D12_SRV_DIMENSION_TEXTURE2D;
}

D3D12_UAV_DIMENSION toUavDim(RhiTextureDimension D, uint32_t ArrayLayers) noexcept
{
    const bool Array = ArrayLayers > 1;
    switch (D)
    {
    case RhiTextureDimension::Tex1D:   return Array ? D3D12_UAV_DIMENSION_TEXTURE1DARRAY : D3D12_UAV_DIMENSION_TEXTURE1D;
    case RhiTextureDimension::Tex2D:   return Array ? D3D12_UAV_DIMENSION_TEXTURE2DARRAY : D3D12_UAV_DIMENSION_TEXTURE2D;
    case RhiTextureDimension::Tex3D:   return D3D12_UAV_DIMENSION_TEXTURE3D;
    case RhiTextureDimension::TexCube: return D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
    }
    return D3D12_UAV_DIMENSION_TEXTURE2D;
}

} // namespace

Rc<D3D12Texture> D3D12Texture::create(D3D12Device* Device, const RhiTextureDesc& Desc) noexcept
{
    if (!Device)
        return {};

    auto Self = Rc<D3D12Texture>(new D3D12Texture{});
    Self->OwnerDevice = Device;
    Self->Desc_       = Desc;

    const D3D12_RESOURCE_DESC   D = makeTexDesc(Desc);
    const D3D12_RESOURCE_STATES InitialStates = D3D12_RESOURCE_STATE_COMMON;
    D3D12_CLEAR_VALUE ClearValue{};
    const bool        HasClear = pickClearValue(Desc, ClearValue);

#if defined(GOLETA_RHID3D12_HAS_D3D12MA) && GOLETA_RHID3D12_HAS_D3D12MA
    if (auto* Alloc = Device->allocator())
    {
        D3D12MA::ALLOCATION_DESC AllocDesc{};
        AllocDesc.HeapType = toHeap(Desc.Location);

        D3D12MA::Allocation* RawAlloc = nullptr;
        ID3D12Resource*      RawRes   = nullptr;
        const HRESULT        Hr       = Alloc->CreateResource(&AllocDesc, &D, InitialStates,
                                                              HasClear ? &ClearValue : nullptr,
                                                              &RawAlloc, IID_PPV_ARGS(&RawRes));
        if (FAILED(Hr))
        {
            GOLETA_LOG_ERROR(D3D12, "D3D12MA::CreateResource(texture): HRESULT 0x{:08x}", static_cast<unsigned>(Hr));
            return {};
        }
        Self->Allocation.Attach(RawAlloc);
        Self->Resource.Attach(RawRes);
    }
    else
#endif
    {
        D3D12_HEAP_PROPERTIES Props{};
        Props.Type = toHeap(Desc.Location);
        const HRESULT Hr = Device->raw()->CreateCommittedResource(&Props, D3D12_HEAP_FLAG_NONE, &D, InitialStates,
                                                                   HasClear ? &ClearValue : nullptr,
                                                                   IID_PPV_ARGS(&Self->Resource));
        if (FAILED(Hr))
        {
            GOLETA_LOG_ERROR(D3D12, "CreateCommittedResource(texture): HRESULT 0x{:08x}", static_cast<unsigned>(Hr));
            return {};
        }
    }

    if (Desc.DebugName)
    {
        Self->Name = Desc.DebugName;
        setD3dObjectName(Self->Resource.Get(), Desc.DebugName);
    }
    Self->allocateDefaultBindlessViews();
    return Self;
}

Rc<D3D12Texture> D3D12Texture::createPlaced(D3D12Device* Device, ID3D12Heap* Heap, uint64_t Offset,
                                            const RhiTextureDesc& Desc) noexcept
{
    if (!Device || !Heap)
        return {};
    auto Self = Rc<D3D12Texture>(new D3D12Texture{});
    Self->OwnerDevice = Device;
    Self->Desc_       = Desc;

    const D3D12_RESOURCE_DESC D = makeTexDesc(Desc);
    D3D12_CLEAR_VALUE Cv{};
    const bool HasClear = pickClearValue(Desc, Cv);
    const HRESULT Hr = Device->raw()->CreatePlacedResource(Heap, Offset, &D, D3D12_RESOURCE_STATE_COMMON,
                                                           HasClear ? &Cv : nullptr, IID_PPV_ARGS(&Self->Resource));
    if (FAILED(Hr))
    {
        GOLETA_LOG_ERROR(D3D12, "CreatePlacedResource(texture): HRESULT 0x{:08x}", static_cast<unsigned>(Hr));
        return {};
    }
    if (Desc.DebugName)
    {
        Self->Name = Desc.DebugName;
        setD3dObjectName(Self->Resource.Get(), Desc.DebugName);
    }
    Self->allocateDefaultBindlessViews();
    return Self;
}

Rc<D3D12Texture> D3D12Texture::wrapRaw(D3D12Device* Device, ComPtr<ID3D12Resource> Resource,
                                       const RhiTextureDesc& Desc) noexcept
{
    auto Self = Rc<D3D12Texture>(new D3D12Texture{});
    Self->OwnerDevice = Device;
    Self->Resource    = std::move(Resource);
    Self->Desc_       = Desc;
    Self->allocateDefaultBindlessViews();
    return Self;
}

D3D12Texture::~D3D12Texture()
{
    if (!OwnerDevice)
        return;
    if (DefaultSrvIndex != InvalidBindlessIndex)
        OwnerDevice->bindless().resourceHeap().free(DefaultSrvIndex);
    if (DefaultUavIndex != InvalidBindlessIndex)
        OwnerDevice->bindless().resourceHeap().free(DefaultUavIndex);
    for (const auto& E : RtvCache)
        if (E.HeapIdx != 0xFFFFFFFFu) OwnerDevice->rtvHeap().free(E.HeapIdx);
    for (const auto& E : DsvCache)
        if (E.HeapIdx != 0xFFFFFFFFu) OwnerDevice->dsvHeap().free(E.HeapIdx);
}

void D3D12Texture::setDebugName(const char* NewName)
{
    Name = NewName ? NewName : "";
    if (Resource)
        setD3dObjectName(Resource.Get(), Name.c_str());
}

void D3D12Texture::allocateDefaultBindlessViews() noexcept
{
    if (!OwnerDevice || !Resource)
        return;
    auto* Dev  = OwnerDevice->raw();
    auto& Heap = OwnerDevice->bindless().resourceHeap();

    if (anyOf(Desc_.Usage, RhiTextureUsage::Sampled))
    {
        const uint32_t Idx = Heap.allocate();
        if (Idx != InvalidBindlessIndex)
        {
            D3D12_SHADER_RESOURCE_VIEW_DESC V{};
            V.Format                  = toDxgiSrv(Desc_.Format);
            V.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            V.ViewDimension           = toSrvDim(Desc_.Dimension, Desc_.DepthOrArrayLayers);
            switch (V.ViewDimension)
            {
            case D3D12_SRV_DIMENSION_TEXTURE2D:        V.Texture2D.MipLevels   = Desc_.MipLevels; break;
            case D3D12_SRV_DIMENSION_TEXTURE2DARRAY:
                V.Texture2DArray.MipLevels = Desc_.MipLevels;
                V.Texture2DArray.ArraySize = Desc_.DepthOrArrayLayers;
                break;
            case D3D12_SRV_DIMENSION_TEXTURE3D:        V.Texture3D.MipLevels   = Desc_.MipLevels; break;
            case D3D12_SRV_DIMENSION_TEXTURECUBE:      V.TextureCube.MipLevels = Desc_.MipLevels; break;
            default: break;
            }
            Dev->CreateShaderResourceView(Resource.Get(), &V, Heap.cpuHandle(Idx));
            DefaultSrvIndex = Idx;
        }
    }
    if (anyOf(Desc_.Usage, RhiTextureUsage::Storage))
    {
        const uint32_t Idx = Heap.allocate();
        if (Idx != InvalidBindlessIndex)
        {
            D3D12_UNORDERED_ACCESS_VIEW_DESC V{};
            V.Format        = toDxgi(Desc_.Format);
            V.ViewDimension = toUavDim(Desc_.Dimension, Desc_.DepthOrArrayLayers);
            switch (V.ViewDimension)
            {
            case D3D12_UAV_DIMENSION_TEXTURE2DARRAY:
                V.Texture2DArray.ArraySize = Desc_.DepthOrArrayLayers;
                break;
            case D3D12_UAV_DIMENSION_TEXTURE3D:
                V.Texture3D.WSize = Desc_.DepthOrArrayLayers;
                break;
            default: break;
            }
            Dev->CreateUnorderedAccessView(Resource.Get(), nullptr, &V, Heap.cpuHandle(Idx));
            DefaultUavIndex = Idx;
        }
    }
}

uint32_t D3D12Texture::ensureRtvIndex(uint32_t MipSlice, uint32_t ArraySlice) noexcept
{
    for (const auto& E : RtvCache)
        if (E.Mip == MipSlice && E.ArraySl == ArraySlice) return E.HeapIdx;
    const uint32_t Idx = OwnerDevice->rtvHeap().allocate();
    if (Idx == 0xFFFFFFFFu)
        return Idx;
    D3D12_RENDER_TARGET_VIEW_DESC V{};
    V.Format        = toDxgi(Desc_.Format);
    if (Desc_.DepthOrArrayLayers > 1)
    {
        V.ViewDimension                     = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
        V.Texture2DArray.MipSlice           = MipSlice;
        V.Texture2DArray.FirstArraySlice    = ArraySlice;
        V.Texture2DArray.ArraySize          = 1;
    }
    else
    {
        V.ViewDimension         = D3D12_RTV_DIMENSION_TEXTURE2D;
        V.Texture2D.MipSlice    = MipSlice;
    }
    OwnerDevice->raw()->CreateRenderTargetView(Resource.Get(), &V, OwnerDevice->rtvHeap().cpuHandle(Idx));
    RtvCache.push_back({MipSlice, ArraySlice, Idx});
    return Idx;
}

uint32_t D3D12Texture::ensureDsvIndex(uint32_t MipSlice, uint32_t ArraySlice) noexcept
{
    for (const auto& E : DsvCache)
        if (E.Mip == MipSlice && E.ArraySl == ArraySlice) return E.HeapIdx;
    const uint32_t Idx = OwnerDevice->dsvHeap().allocate();
    if (Idx == 0xFFFFFFFFu)
        return Idx;
    D3D12_DEPTH_STENCIL_VIEW_DESC V{};
    V.Format        = toDxgiDsv(Desc_.Format);
    if (Desc_.DepthOrArrayLayers > 1)
    {
        V.ViewDimension                  = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
        V.Texture2DArray.MipSlice        = MipSlice;
        V.Texture2DArray.FirstArraySlice = ArraySlice;
        V.Texture2DArray.ArraySize       = 1;
    }
    else
    {
        V.ViewDimension      = D3D12_DSV_DIMENSION_TEXTURE2D;
        V.Texture2D.MipSlice = MipSlice;
    }
    OwnerDevice->raw()->CreateDepthStencilView(Resource.Get(), &V, OwnerDevice->dsvHeap().cpuHandle(Idx));
    DsvCache.push_back({MipSlice, ArraySlice, Idx});
    return Idx;
}

// ---------- D3D12TextureView ----------

Rc<D3D12TextureView> D3D12TextureView::create(D3D12Device* Device, const RhiTextureViewDesc& Desc) noexcept
{
    if (!Device || !Desc.Texture)
        return {};

    auto Self = Rc<D3D12TextureView>(new D3D12TextureView{});
    Self->OwnerDevice = Device;
    Self->Owner       = Rc<IRhiTexture>(Desc.Texture);
    Self->Desc_       = Desc;

    auto* Tex = d3d12Cast<D3D12Texture>(Desc.Texture);
    const RhiFormat FormatIn  = Desc.Format == RhiFormat::Unknown ? Tex->desc().Format : Desc.Format;
    auto&          Heap       = Device->bindless().resourceHeap();
    const uint32_t Mips       = Desc.MipLevelCount == 0   ? Tex->desc().MipLevels - Desc.BaseMipLevel : Desc.MipLevelCount;
    const uint32_t Layers     = Desc.ArrayLayerCount == 0 ? Tex->desc().DepthOrArrayLayers - Desc.BaseArrayLayer
                                                           : Desc.ArrayLayerCount;

    if (Desc.WritableUav)
    {
        const uint32_t Idx = Heap.allocate();
        if (Idx != InvalidBindlessIndex)
        {
            D3D12_UNORDERED_ACCESS_VIEW_DESC V{};
            V.Format        = toDxgi(FormatIn);
            V.ViewDimension = toUavDim(Desc.Dimension, Layers);
            switch (V.ViewDimension)
            {
            case D3D12_UAV_DIMENSION_TEXTURE2D:
                V.Texture2D.MipSlice = Desc.BaseMipLevel;
                break;
            case D3D12_UAV_DIMENSION_TEXTURE2DARRAY:
                V.Texture2DArray.MipSlice        = Desc.BaseMipLevel;
                V.Texture2DArray.FirstArraySlice = Desc.BaseArrayLayer;
                V.Texture2DArray.ArraySize       = Layers;
                break;
            case D3D12_UAV_DIMENSION_TEXTURE3D:
                V.Texture3D.MipSlice = Desc.BaseMipLevel;
                V.Texture3D.WSize    = Layers;
                break;
            default: break;
            }
            Device->raw()->CreateUnorderedAccessView(Tex->raw(), nullptr, &V, Heap.cpuHandle(Idx));
            Self->UavIndex = Idx;
        }
    }
    else
    {
        const uint32_t Idx = Heap.allocate();
        if (Idx != InvalidBindlessIndex)
        {
            D3D12_SHADER_RESOURCE_VIEW_DESC V{};
            V.Format                  = toDxgiSrv(FormatIn);
            V.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            V.ViewDimension           = toSrvDim(Desc.Dimension, Layers);
            switch (V.ViewDimension)
            {
            case D3D12_SRV_DIMENSION_TEXTURE2D:
                V.Texture2D.MostDetailedMip = Desc.BaseMipLevel;
                V.Texture2D.MipLevels       = Mips;
                break;
            case D3D12_SRV_DIMENSION_TEXTURE2DARRAY:
                V.Texture2DArray.MostDetailedMip = Desc.BaseMipLevel;
                V.Texture2DArray.MipLevels       = Mips;
                V.Texture2DArray.FirstArraySlice = Desc.BaseArrayLayer;
                V.Texture2DArray.ArraySize       = Layers;
                break;
            case D3D12_SRV_DIMENSION_TEXTURE3D:
                V.Texture3D.MostDetailedMip = Desc.BaseMipLevel;
                V.Texture3D.MipLevels       = Mips;
                break;
            case D3D12_SRV_DIMENSION_TEXTURECUBE:
                V.TextureCube.MostDetailedMip = Desc.BaseMipLevel;
                V.TextureCube.MipLevels       = Mips;
                break;
            default: break;
            }
            Device->raw()->CreateShaderResourceView(Tex->raw(), &V, Heap.cpuHandle(Idx));
            Self->SrvIndex = Idx;
        }
    }

    if (Desc.DebugName)
        Self->Name = Desc.DebugName;
    return Self;
}

D3D12TextureView::~D3D12TextureView()
{
    if (!OwnerDevice)
        return;
    if (SrvIndex != InvalidBindlessIndex)
        OwnerDevice->bindless().resourceHeap().free(SrvIndex);
    if (UavIndex != InvalidBindlessIndex)
        OwnerDevice->bindless().resourceHeap().free(UavIndex);
}

} // namespace goleta::rhi::d3d12
