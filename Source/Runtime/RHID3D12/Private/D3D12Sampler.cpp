/// @file
/// @brief IRhiSampler over shader-visible sampler heap.

#include "D3D12Sampler.h"

#include "D3D12Device.h"

namespace goleta::rhi::d3d12
{
namespace
{

D3D12_FILTER toD3dFilter(const RhiSamplerDesc& D) noexcept
{
    const bool Cmp = D.CompareOp != RhiCompareOp::Never;
    if (D.MaxAnisotropy > 1)
        return Cmp ? D3D12_FILTER_COMPARISON_ANISOTROPIC : D3D12_FILTER_ANISOTROPIC;

    const int Bits = ((D.MinFilter == RhiFilter::Linear ? 1 : 0) << 2) |
                     ((D.MagFilter == RhiFilter::Linear ? 1 : 0) << 1) |
                     (D.MipFilter == RhiFilter::Linear ? 1 : 0);
    static constexpr D3D12_FILTER kTable[8] = {
        D3D12_FILTER_MIN_MAG_MIP_POINT,
        D3D12_FILTER_MIN_MAG_POINT_MIP_LINEAR,
        D3D12_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT,
        D3D12_FILTER_MIN_POINT_MAG_MIP_LINEAR,
        D3D12_FILTER_MIN_LINEAR_MAG_MIP_POINT,
        D3D12_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR,
        D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT,
        D3D12_FILTER_MIN_MAG_MIP_LINEAR,
    };
    static constexpr D3D12_FILTER kCmpTable[8] = {
        D3D12_FILTER_COMPARISON_MIN_MAG_MIP_POINT,
        D3D12_FILTER_COMPARISON_MIN_MAG_POINT_MIP_LINEAR,
        D3D12_FILTER_COMPARISON_MIN_POINT_MAG_LINEAR_MIP_POINT,
        D3D12_FILTER_COMPARISON_MIN_POINT_MAG_MIP_LINEAR,
        D3D12_FILTER_COMPARISON_MIN_LINEAR_MAG_MIP_POINT,
        D3D12_FILTER_COMPARISON_MIN_LINEAR_MAG_POINT_MIP_LINEAR,
        D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT,
        D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR,
    };
    return Cmp ? kCmpTable[Bits] : kTable[Bits];
}

D3D12_TEXTURE_ADDRESS_MODE toAddress(RhiSamplerAddressMode M) noexcept
{
    switch (M)
    {
    case RhiSamplerAddressMode::Repeat:        return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    case RhiSamplerAddressMode::MirrorRepeat:  return D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
    case RhiSamplerAddressMode::ClampToEdge:   return D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    case RhiSamplerAddressMode::ClampToBorder: return D3D12_TEXTURE_ADDRESS_MODE_BORDER;
    }
    return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
}

D3D12_COMPARISON_FUNC toCompare(RhiCompareOp C) noexcept
{
    switch (C)
    {
    case RhiCompareOp::Never:        return D3D12_COMPARISON_FUNC_NEVER;
    case RhiCompareOp::Less:         return D3D12_COMPARISON_FUNC_LESS;
    case RhiCompareOp::Equal:        return D3D12_COMPARISON_FUNC_EQUAL;
    case RhiCompareOp::LessEqual:    return D3D12_COMPARISON_FUNC_LESS_EQUAL;
    case RhiCompareOp::Greater:      return D3D12_COMPARISON_FUNC_GREATER;
    case RhiCompareOp::NotEqual:     return D3D12_COMPARISON_FUNC_NOT_EQUAL;
    case RhiCompareOp::GreaterEqual: return D3D12_COMPARISON_FUNC_GREATER_EQUAL;
    case RhiCompareOp::Always:       return D3D12_COMPARISON_FUNC_ALWAYS;
    }
    return D3D12_COMPARISON_FUNC_NEVER;
}

void fillBorderColor(RhiBorderColor C, float Out[4]) noexcept
{
    switch (C)
    {
    case RhiBorderColor::TransparentBlack: Out[0]=Out[1]=Out[2]=Out[3]=0.0f; break;
    case RhiBorderColor::OpaqueBlack:      Out[0]=Out[1]=Out[2]=0.0f; Out[3]=1.0f; break;
    case RhiBorderColor::OpaqueWhite:      Out[0]=Out[1]=Out[2]=Out[3]=1.0f; break;
    }
}

} // namespace

Rc<D3D12Sampler> D3D12Sampler::create(D3D12Device* Device, const RhiSamplerDesc& Desc) noexcept
{
    if (!Device)
        return {};
    auto Self = Rc<D3D12Sampler>(new D3D12Sampler{});
    Self->OwnerDevice = Device;
    Self->Desc_       = Desc;

    D3D12_SAMPLER_DESC& D = Self->D3dDesc;
    D                = {};
    D.Filter         = toD3dFilter(Desc);
    D.AddressU       = toAddress(Desc.AddressU);
    D.AddressV       = toAddress(Desc.AddressV);
    D.AddressW       = toAddress(Desc.AddressW);
    D.MipLODBias     = Desc.MipLodBias;
    D.MaxAnisotropy  = Desc.MaxAnisotropy;
    D.ComparisonFunc = toCompare(Desc.CompareOp);
    fillBorderColor(Desc.BorderColor, D.BorderColor);
    D.MinLOD         = Desc.MinLod;
    D.MaxLOD         = Desc.MaxLod;

    const uint32_t Slot = Device->bindless().samplerHeap().allocate();
    if (Slot == InvalidBindlessIndex)
    {
        GOLETA_LOG_ERROR(D3D12, "Sampler heap exhausted");
        return {};
    }
    Device->raw()->CreateSampler(&D, Device->bindless().samplerHeap().cpuHandle(Slot));
    Self->Index = Slot;

    if (Desc.DebugName)
        Self->Name = Desc.DebugName;
    return Self;
}

D3D12Sampler::~D3D12Sampler()
{
    if (OwnerDevice && Index != InvalidBindlessIndex)
        OwnerDevice->bindless().samplerHeap().free(Index);
}

} // namespace goleta::rhi::d3d12
