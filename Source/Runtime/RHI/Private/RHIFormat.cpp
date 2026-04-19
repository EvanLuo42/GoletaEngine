/// @file
/// @brief Format helpers — pure tables, no backend dependency.

#include "RHIFormat.h"

namespace goleta::rhi
{
namespace
{

constexpr uint32_t kFormatSizeTable[] = {
    /* Unknown */ 0,

    /* R8Unorm */ 1,
    /* R8Snorm */ 1,
    /* R8Uint */ 1,
    /* R8Sint */ 1,

    /* Rg8Unorm */ 2,
    /* Rg8Snorm */ 2,
    /* Rg8Uint */ 2,
    /* Rg8Sint */ 2,

    /* Rgba8Unorm */ 4,
    /* Rgba8UnormSrgb */ 4,
    /* Rgba8Snorm */ 4,
    /* Rgba8Uint */ 4,
    /* Rgba8Sint */ 4,
    /* Bgra8Unorm */ 4,
    /* Bgra8UnormSrgb */ 4,

    /* R16Unorm */ 2,
    /* R16Snorm */ 2,
    /* R16Uint */ 2,
    /* R16Sint */ 2,
    /* R16Float */ 2,

    /* Rg16Unorm */ 4,
    /* Rg16Snorm */ 4,
    /* Rg16Uint */ 4,
    /* Rg16Sint */ 4,
    /* Rg16Float */ 4,

    /* Rgba16Unorm */ 8,
    /* Rgba16Snorm */ 8,
    /* Rgba16Uint */ 8,
    /* Rgba16Sint */ 8,
    /* Rgba16Float */ 8,

    /* R32Uint */ 4,
    /* R32Sint */ 4,
    /* R32Float */ 4,

    /* Rg32Uint */ 8,
    /* Rg32Sint */ 8,
    /* Rg32Float */ 8,

    /* Rgb32Uint */ 12,
    /* Rgb32Sint */ 12,
    /* Rgb32Float */ 12,

    /* Rgba32Uint */ 16,
    /* Rgba32Sint */ 16,
    /* Rgba32Float */ 16,

    /* R11G11B10Float */ 4,
    /* Rgb10A2Unorm */ 4,
    /* Rgb10A2Uint */ 4,
    /* Rgb9E5UFloat */ 4,

    /* D16Unorm */ 2,
    /* D24UnormS8Uint */ 4,
    /* D32Float */ 4,
    /* D32FloatS8Uint */ 8, // 32-bit depth + 8-bit stencil, padded.

    /* Bc1RgbUnorm */ 8,
    /* Bc1RgbUnormSrgb */ 8,
    /* Bc1RgbaUnorm */ 8,
    /* Bc1RgbaUnormSrgb */ 8,
    /* Bc2RgbaUnorm */ 16,
    /* Bc2RgbaUnormSrgb */ 16,
    /* Bc3RgbaUnorm */ 16,
    /* Bc3RgbaUnormSrgb */ 16,
    /* Bc4RUnorm */ 8,
    /* Bc4RSnorm */ 8,
    /* Bc5RgUnorm */ 16,
    /* Bc5RgSnorm */ 16,
    /* Bc6HRgbUFloat */ 16,
    /* Bc6HRgbSFloat */ 16,
    /* Bc7RgbaUnorm */ 16,
    /* Bc7RgbaUnormSrgb */ 16,
};

static_assert(sizeof(kFormatSizeTable) / sizeof(kFormatSizeTable[0]) == static_cast<size_t>(RhiFormat::Count),
              "RhiFormat size table out of sync with RhiFormat enum");

} // namespace

uint32_t formatSizeBytes(RhiFormat Format) noexcept
{
    const size_t Index = static_cast<size_t>(Format);
    if (Index >= static_cast<size_t>(RhiFormat::Count))
        return 0;
    return kFormatSizeTable[Index];
}

void formatBlockExtent(const RhiFormat Format, uint32_t& OutWidth, uint32_t& OutHeight) noexcept
{
    if (formatIsBlockCompressed(Format))
    {
        OutWidth  = 4;
        OutHeight = 4;
    }
    else
    {
        OutWidth  = 1;
        OutHeight = 1;
    }
}

bool formatIsDepth(const RhiFormat Format) noexcept
{
    switch (Format)
    {
    case RhiFormat::D16Unorm:
    case RhiFormat::D24UnormS8Uint:
    case RhiFormat::D32Float:
    case RhiFormat::D32FloatS8Uint:
        return true;
    default:
        return false;
    }
}

bool formatIsStencil(const RhiFormat Format) noexcept
{
    switch (Format)
    {
    case RhiFormat::D24UnormS8Uint:
    case RhiFormat::D32FloatS8Uint:
        return true;
    default:
        return false;
    }
}

bool formatIsSrgb(const RhiFormat Format) noexcept
{
    switch (Format)
    {
    case RhiFormat::Rgba8UnormSrgb:
    case RhiFormat::Bgra8UnormSrgb:
    case RhiFormat::Bc1RgbUnormSrgb:
    case RhiFormat::Bc1RgbaUnormSrgb:
    case RhiFormat::Bc2RgbaUnormSrgb:
    case RhiFormat::Bc3RgbaUnormSrgb:
    case RhiFormat::Bc7RgbaUnormSrgb:
        return true;
    default:
        return false;
    }
}

bool formatIsBlockCompressed(const RhiFormat Format) noexcept
{
    switch (Format)
    {
    case RhiFormat::Bc1RgbUnorm:
    case RhiFormat::Bc1RgbUnormSrgb:
    case RhiFormat::Bc1RgbaUnorm:
    case RhiFormat::Bc1RgbaUnormSrgb:
    case RhiFormat::Bc2RgbaUnorm:
    case RhiFormat::Bc2RgbaUnormSrgb:
    case RhiFormat::Bc3RgbaUnorm:
    case RhiFormat::Bc3RgbaUnormSrgb:
    case RhiFormat::Bc4RUnorm:
    case RhiFormat::Bc4RSnorm:
    case RhiFormat::Bc5RgUnorm:
    case RhiFormat::Bc5RgSnorm:
    case RhiFormat::Bc6HRgbUFloat:
    case RhiFormat::Bc6HRgbSFloat:
    case RhiFormat::Bc7RgbaUnorm:
    case RhiFormat::Bc7RgbaUnormSrgb:
        return true;
    default:
        return false;
    }
}

} // namespace goleta::rhi
