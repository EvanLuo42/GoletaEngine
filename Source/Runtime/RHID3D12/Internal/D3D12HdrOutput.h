#pragma once

/// @file
/// @brief SCAFFOLD — HDR swap-chain color-space configuration. Swap chain rejects HdrOutput=true
///        for MVP; the real color-space setup happens here when features().HdrOutput flips on.

#include "D3D12Prelude.h"

namespace goleta::rhi::d3d12
{
// TODO(rhi): IDXGISwapChain4::SetColorSpace1 + HDR metadata.
} // namespace goleta::rhi::d3d12
