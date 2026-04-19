#pragma once

/// @file
/// @brief Pixel / vertex-attribute format enum plus pure helpers (size, depth, sRGB).

#include <cstdint>

#include "RHIExport.h"

namespace goleta::rhi
{

/// @brief Cross-API texel / vertex format. Values mirror the DXGI_FORMAT / VkFormat families
///        so individual backends can hold small translation tables. Do not rely on numeric
///        values matching either native API.
enum class RhiFormat : uint32_t
{
    Unknown = 0,

    R8Unorm,
    R8Snorm,
    R8Uint,
    R8Sint,

    Rg8Unorm,
    Rg8Snorm,
    Rg8Uint,
    Rg8Sint,

    Rgba8Unorm,
    Rgba8UnormSrgb,
    Rgba8Snorm,
    Rgba8Uint,
    Rgba8Sint,
    Bgra8Unorm,
    Bgra8UnormSrgb,

    R16Unorm,
    R16Snorm,
    R16Uint,
    R16Sint,
    R16Float,

    Rg16Unorm,
    Rg16Snorm,
    Rg16Uint,
    Rg16Sint,
    Rg16Float,

    Rgba16Unorm,
    Rgba16Snorm,
    Rgba16Uint,
    Rgba16Sint,
    Rgba16Float,

    R32Uint,
    R32Sint,
    R32Float,

    Rg32Uint,
    Rg32Sint,
    Rg32Float,

    Rgb32Uint,
    Rgb32Sint,
    Rgb32Float,

    Rgba32Uint,
    Rgba32Sint,
    Rgba32Float,

    R11G11B10Float,
    Rgb10A2Unorm,
    Rgb10A2Uint,
    Rgb9E5UFloat,

    D16Unorm,
    D24UnormS8Uint,
    D32Float,
    D32FloatS8Uint,

    Bc1RgbUnorm,
    Bc1RgbUnormSrgb,
    Bc1RgbaUnorm,
    Bc1RgbaUnormSrgb,
    Bc2RgbaUnorm,
    Bc2RgbaUnormSrgb,
    Bc3RgbaUnorm,
    Bc3RgbaUnormSrgb,
    Bc4RUnorm,
    Bc4RSnorm,
    Bc5RgUnorm,
    Bc5RgSnorm,
    Bc6HRgbUFloat,
    Bc6HRgbSFloat,
    Bc7RgbaUnorm,
    Bc7RgbaUnormSrgb,

    Count,
};

/// @brief Per-format capability bits reported by IRhiDevice::features().formatCaps().
enum class RhiFormatCaps : uint32_t
{
    None            = 0,
    Sampled         = 1u << 0,
    SampledFiltered = 1u << 1,
    Storage         = 1u << 2,
    StorageAtomic   = 1u << 3,
    ColorAttachment = 1u << 4,
    ColorBlend      = 1u << 5,
    DepthAttachment = 1u << 6,
    VertexAttribute = 1u << 7,
    CopySource      = 1u << 8,
    CopyDest        = 1u << 9,
};

constexpr RhiFormatCaps operator|(RhiFormatCaps A, RhiFormatCaps B) noexcept
{
    return static_cast<RhiFormatCaps>(static_cast<uint32_t>(A) | static_cast<uint32_t>(B));
}
constexpr RhiFormatCaps operator&(RhiFormatCaps A, RhiFormatCaps B) noexcept
{
    return static_cast<RhiFormatCaps>(static_cast<uint32_t>(A) & static_cast<uint32_t>(B));
}
constexpr bool anyOf(RhiFormatCaps Bits, RhiFormatCaps Mask) noexcept
{
    return (static_cast<uint32_t>(Bits) & static_cast<uint32_t>(Mask)) != 0;
}

/// @brief Bytes occupied by one texel of Format. For block-compressed formats, this returns
///        the bytes per block and formatBlockExtent() reports the block footprint.
/// @return Size in bytes, or 0 for Unknown.
RHI_API uint32_t formatSizeBytes(RhiFormat Format) noexcept;

/// @brief Block footprint of a compressed format. Returns (1,1) for uncompressed formats.
RHI_API void formatBlockExtent(RhiFormat Format, uint32_t& OutWidth, uint32_t& OutHeight) noexcept;

/// @brief Whether Format carries depth information.
RHI_API bool formatIsDepth(RhiFormat Format) noexcept;

/// @brief Whether Format carries a stencil channel.
RHI_API bool formatIsStencil(RhiFormat Format) noexcept;

/// @brief Whether Format is sRGB-encoded.
RHI_API bool formatIsSrgb(RhiFormat Format) noexcept;

/// @brief Whether Format is a BCn block-compressed format.
RHI_API bool formatIsBlockCompressed(RhiFormat Format) noexcept;

} // namespace goleta::rhi
