#pragma once

/// @file
/// @brief Texture resource interface, texture-view interface, and their creation descriptors.

#include <cstdint>

#include "RHIEnums.h"
#include "RHIExport.h"
#include "RHIFormat.h"
#include "RHIHandle.h"
#include "RHIResource.h"
#include "RHIStructChain.h"

namespace goleta::rhi
{

/// @brief Clear-value payload associated with a texture for fast optimized clears.
struct RhiClearValue
{
    float    Color[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    float    Depth    = 1.0f;
    uint32_t Stencil  = 0;
    bool     UseColor = true; // false => depth/stencil clear.
};

/// @brief Creation parameters for a texture.
struct RhiTextureDesc
{
    static constexpr auto kStructType = RhiStructType::TextureDesc;
    RhiStructHeader       Header{kStructType, nullptr};

    RhiTextureDimension Dimension          = RhiTextureDimension::Tex2D;
    RhiFormat           Format             = RhiFormat::Rgba8Unorm;
    uint32_t            Width              = 1;
    uint32_t            Height             = 1;
    uint32_t            DepthOrArrayLayers = 1;
    uint32_t            MipLevels          = 1;
    RhiSampleCount      Samples            = RhiSampleCount::X1;
    RhiTextureUsage     Usage              = RhiTextureUsage::Sampled;
    RhiMemoryLocation   Location           = RhiMemoryLocation::DeviceLocal;
    RhiClearValue       OptimizedClearValue{};

    const char* DebugName = nullptr;
};

class IRhiTexture;

/// @brief Creation parameters for a typed view over a texture's subresource range.
struct RhiTextureViewDesc
{
    IRhiTexture*        Texture         = nullptr;            // Non-owning; view holds a strong ref.
    RhiFormat           Format          = RhiFormat::Unknown; // Unknown == inherit from texture.
    RhiTextureDimension Dimension       = RhiTextureDimension::Tex2D;
    uint32_t            BaseMipLevel    = 0;
    uint32_t            MipLevelCount   = 0; // 0 == remaining.
    uint32_t            BaseArrayLayer  = 0;
    uint32_t            ArrayLayerCount = 0; // 0 == remaining.

    /// @brief If true, the view binds as UAV (requires Storage in texture usage); otherwise SRV.
    bool WritableUav = false;

    const char* DebugName = nullptr;
};

/// @brief Typed, bindable view over a texture subresource range. Distinct Rc<>-managed object
///        so that a single texture can expose multiple mip/array/format views.
class RHI_API IRhiTextureView : public IRhiResource
{
public:
    virtual const RhiTextureViewDesc& desc() const noexcept    = 0;
    virtual IRhiTexture*              texture() const noexcept = 0;

    virtual RhiTextureHandle   srvHandle() const noexcept = 0;
    virtual RhiRwTextureHandle uavHandle() const noexcept = 0;
};

/// @brief GPU texture. Default SRV over the whole resource is exposed via srvHandle().
class RHI_API IRhiTexture : public IRhiResource
{
public:
    virtual const RhiTextureDesc& desc() const noexcept = 0;

    /// @brief Bindless SRV over the whole resource. Convenience; equivalent to creating a view
    ///        with BaseMipLevel=0, MipLevelCount=all, BaseArrayLayer=0, ArrayLayerCount=all.
    virtual RhiTextureHandle srvHandle() const noexcept = 0;

    /// @brief Bindless UAV over mip 0 (requires Storage usage).
    virtual RhiRwTextureHandle uavHandle() const noexcept = 0;
};

} // namespace goleta::rhi
