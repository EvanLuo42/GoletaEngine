#pragma once

/// @file
/// @brief Mesh-shading is expressed through RhiGraphicsPipelineDesc's Mesh/Amplification fields
///        and IRhiCommandList::drawMeshTasks. This header reserves space for future
///        mesh-shader-only interfaces (e.g. indirect count buffers, work-graph integration).

#include "RHIExport.h"

namespace goleta::rhi
{

/// @brief Whether the device supports the mesh-shading extension.
/// @return Matches IRhiDevice::features().MeshShading.Supported; this free function exists
///         only as a forward-declared convenience point — no dedicated interface object is needed.

} // namespace goleta::rhi
