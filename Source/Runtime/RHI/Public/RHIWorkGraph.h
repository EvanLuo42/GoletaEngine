#pragma once

/// @file
/// @brief Work graphs (D3D12 SM 6.8+). Placeholder for future dedicated interfaces. For now
///        the concept is reported via IRhiDevice::features().WorkGraphs and would use a
///        specialized pipeline desc in a follow-up task.

#include "RHIExport.h"

namespace goleta::rhi
{

/// @brief Work graphs are intentionally stubbed at the interface level — the final ABI is
///        still in flight on D3D12 and has no Vulkan counterpart as of the cutoff. Consumers
///        should feature-gate on features().WorkGraphs.Supported and expect follow-up work
///        to add IRhiWorkGraph / IRhiWorkGraphPipeline here.

} // namespace goleta::rhi
