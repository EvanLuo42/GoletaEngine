#pragma once

/// @file
/// @brief Sampler resource interface and creation descriptor.

#include <cstdint>

#include "RHIEnums.h"
#include "RHIExport.h"
#include "RHIHandle.h"
#include "RHIResource.h"
#include "RHIStructChain.h"

namespace goleta::rhi
{

/// @brief Creation parameters for a sampler.
struct RhiSamplerDesc
{
    static constexpr auto kStructType = RhiStructType::SamplerDesc;
    RhiStructHeader       Header{kStructType, nullptr};

    RhiFilter             MinFilter     = RhiFilter::Linear;
    RhiFilter             MagFilter     = RhiFilter::Linear;
    RhiFilter             MipFilter     = RhiFilter::Linear;
    RhiSamplerAddressMode AddressU      = RhiSamplerAddressMode::Repeat;
    RhiSamplerAddressMode AddressV      = RhiSamplerAddressMode::Repeat;
    RhiSamplerAddressMode AddressW      = RhiSamplerAddressMode::Repeat;
    float                 MipLodBias    = 0.0f;
    uint32_t              MaxAnisotropy = 1;
    RhiCompareOp          CompareOp     = RhiCompareOp::Never; // Never disables comparison sampling.
    float                 MinLod        = 0.0f;
    float                 MaxLod        = 3.402823466e+38f; // FLT_MAX
    RhiBorderColor        BorderColor   = RhiBorderColor::TransparentBlack;

    const char* DebugName = nullptr;
};

/// @brief GPU sampler object; bindless-addressable via samplerHandle().
class RHI_API IRhiSampler : public IRhiResource
{
public:
    virtual const RhiSamplerDesc& desc() const noexcept          = 0;
    virtual RhiSamplerHandle      samplerHandle() const noexcept = 0;
};

} // namespace goleta::rhi
