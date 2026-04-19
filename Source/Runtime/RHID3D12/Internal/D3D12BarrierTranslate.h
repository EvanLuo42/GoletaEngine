#pragma once

/// @file
/// @brief Enhanced-Barriers mapping between RHI enums and D3D12 barrier types.

#include "D3D12Prelude.h"
#include "RHIBarrier.h"

namespace goleta::rhi::d3d12
{

D3D12_BARRIER_SYNC   toD3dSync(RhiPipelineStage Stages) noexcept;
D3D12_BARRIER_ACCESS toD3dAccess(RhiAccess Access) noexcept;
D3D12_BARRIER_LAYOUT toD3dLayout(RhiTextureLayout Layout) noexcept;

} // namespace goleta::rhi::d3d12
