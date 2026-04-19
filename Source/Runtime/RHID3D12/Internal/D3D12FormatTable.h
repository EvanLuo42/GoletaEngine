#pragma once

/// @file
/// @brief RhiFormat <-> DXGI_FORMAT translation and per-format plumbing helpers.

#include "D3D12Prelude.h"
#include "RHIFormat.h"

namespace goleta::rhi::d3d12
{

/// @brief Typed DXGI_FORMAT for a given RHI format. DXGI_FORMAT_UNKNOWN for Unknown / Count.
DXGI_FORMAT toDxgi(RhiFormat Format) noexcept;

/// @brief SRV-friendly DXGI_FORMAT (e.g. R24G8_TYPELESS → R24_UNORM_X8_TYPELESS for depth).
DXGI_FORMAT toDxgiSrv(RhiFormat Format) noexcept;

/// @brief DSV-friendly DXGI_FORMAT. DXGI_FORMAT_UNKNOWN when not depth/stencil.
DXGI_FORMAT toDxgiDsv(RhiFormat Format) noexcept;

/// @brief Typeless sibling of the format (needed when the same resource is aliased as
///        depth + SRV). Equals toDxgi() when no typeless variant exists.
DXGI_FORMAT toDxgiTypeless(RhiFormat Format) noexcept;

} // namespace goleta::rhi::d3d12
