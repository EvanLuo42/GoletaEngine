#pragma once

/// @file
/// @brief SCAFFOLD — mesh / amplification shader pipeline plumbing. Not yet implemented.
///        Graphics pipelines ignore Desc.MeshShader / Desc.AmplificationShader for MVP.

#include "D3D12Prelude.h"
#include "RHIMeshShading.h"

namespace goleta::rhi::d3d12
{
// TODO(rhi): implement mesh-shader pipeline build (D3D12_PIPELINE_STATE_STREAM with MS/AS subobjects)
//            + D3D12CommandList::drawMeshTasks override dispatching DispatchMesh.
} // namespace goleta::rhi::d3d12
