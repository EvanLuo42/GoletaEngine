#pragma once

/// @file
/// @brief Translation from (RgBindAs, RgAccessMode) to (RhiTextureLayout, RhiAccess,
///        RhiPipelineStage). Implementation detail of BarrierPlanner.

#include "RHIBarrier.h"
#include "RenderGraphTypes.h"

namespace goleta::rg::detail
{

struct TextureAccessState
{
    rhi::RhiTextureLayout Layout = rhi::RhiTextureLayout::Undefined;
    rhi::RhiAccess        Access = rhi::RhiAccess::None;
    rhi::RhiPipelineStage Stages = rhi::RhiPipelineStage::None;
    bool                  Writes = false;
};

struct BufferAccessState
{
    rhi::RhiAccess        Access = rhi::RhiAccess::None;
    rhi::RhiPipelineStage Stages = rhi::RhiPipelineStage::None;
    bool                  Writes = false;
};

/// @brief Resolve the effective bind for a texture from its declared BindAs and usage bits.
[[nodiscard]] RgBindAs inferTextureBind(RgBindAs Declared, rhi::RhiTextureUsage Usage) noexcept;

/// @brief Map a texture field's access mode + bind to its RHI state requirements.
[[nodiscard]] TextureAccessState textureStateFor(RgBindAs Bind, RgAccessMode Mode) noexcept;

/// @brief Map a buffer field's access mode + bind to its RHI state requirements.
[[nodiscard]] BufferAccessState bufferStateFor(RgBindAs Bind, RgAccessMode Mode,
                                               rhi::RhiBufferUsage Usage) noexcept;

} // namespace goleta::rg::detail
