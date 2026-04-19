#pragma once

/// @file
/// @brief SCAFFOLD — D3D12 ray-tracing interface (DXR 1.1). Not yet implemented.
///        Device::createRayTracingPipeline / createAccelStructure currently return null and
///        feature flags report Supported=false. Fill in here when ready.

#include "D3D12Prelude.h"
#include "RHIRayTracing.h"

namespace goleta::rhi::d3d12
{

class D3D12Device;

// TODO(rhi): implement ray-tracing — D3D12RayTracingPipeline + D3D12AccelStructure classes,
// DXR state-object creation, BLAS/TLAS build routines, SBT handle extraction.

} // namespace goleta::rhi::d3d12
