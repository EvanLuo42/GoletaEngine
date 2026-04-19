#pragma once

/// @file
/// @brief Device-level capability struct queried once at startup and cached by the renderer.

#include <cstdint>

#include "RHIFormat.h"

namespace goleta::rhi
{

/// @brief Immutable snapshot of what a device supports. Renderers call once, cache, and
///        branch at pipeline-creation time — never on the hot draw path.
struct RhiDeviceFeatures
{
    /// @brief Ray tracing support.
    struct RayTracingCaps
    {
        bool     Supported              = false;
        bool     RayQuery               = false; // Inline ray queries inside graphics/compute.
        bool     InlineRayTracing       = false; // SM 6.5 Inline RT.
        bool     ShaderExecutionReorder = false; // SER / VK_NV_ray_tracing_invocation_reorder.
        uint32_t MaxRayRecursionDepth   = 0;
    };
    RayTracingCaps RayTracing;

    /// @brief Mesh / amplification shader support.
    struct MeshShadingCaps
    {
        bool     Supported               = false;
        uint32_t MaxMeshOutputVertices   = 0;
        uint32_t MaxMeshOutputPrimitives = 0;
        uint32_t MaxMeshWorkgroupCountX  = 0;
        uint32_t MaxTaskWorkgroupCountX  = 0;
    };
    MeshShadingCaps MeshShading;

    /// @brief Variable-rate shading.
    struct VrsCaps
    {
        bool     Supported    = false;
        bool     PerPrimitive = false;
        bool     PerTile      = false;
        uint32_t TileSize     = 0; // 0 if per-tile not supported.
    };
    VrsCaps VariableRateShading;

    /// @brief Work-graph support (D3D12 SM 6.8 + Preview).
    struct WorkGraphCaps
    {
        bool Supported = false;
    };
    WorkGraphCaps WorkGraphs;

    struct SamplerFeedbackCaps
    {
        bool Supported = false;
    };
    SamplerFeedbackCaps SamplerFeedback;

    struct SparseCaps
    {
        bool Supported = false;
    };
    SparseCaps SparseBinding;

    struct HdrCaps
    {
        bool Supported    = false;
        bool Rec2020Pq    = false;
        bool ScRgbFloat16 = false;
    };
    HdrCaps HdrOutput;

    /// @brief Bindless descriptor support. Targets of the renderer's default binding model.
    struct BindlessCaps
    {
        bool     Supported         = false;
        uint32_t MaxSrvDescriptors = 0;
        uint32_t MaxUavDescriptors = 0;
        uint32_t MaxSamplers       = 0;
    };
    BindlessCaps BindlessResources;

    /// @brief Core-API version / flavor details. All present on D3D12 12.2+ / Vulkan 1.3+.
    struct CoreCaps
    {
        bool TimelineSemaphore             = false;
        bool EnhancedBarriers              = false;
        bool DynamicRendering              = false;
        bool ShaderModel66DynamicResources = false;
        bool DescriptorBuffer              = false;
    };
    CoreCaps Core;

    /// @brief Limits that rendering code wants to know at pipeline creation.
    uint32_t MaxTextureDimension1D     = 0;
    uint32_t MaxTextureDimension2D     = 0;
    uint32_t MaxTextureDimension3D     = 0;
    uint32_t MaxTextureDimensionCube   = 0;
    uint32_t MaxTextureArrayLayers     = 0;
    uint32_t MaxColorAttachments       = 0;
    uint32_t MaxViewports              = 0;
    uint32_t MaxComputeWorkgroupCountX = 0;
    uint32_t MaxComputeWorkgroupSizeX  = 0;
    uint32_t MaxPushConstantBytes      = 0;
    uint32_t ConstantBufferAlignment   = 0;
    uint32_t StorageBufferAlignment    = 0;
    uint32_t TexelBufferAlignment      = 0;
    uint64_t NonCoherentAtomSize       = 0;

    /// @brief Per-format capability lookup.
    /// @note  Implemented by the backend; lives behind a function pointer so this struct stays trivially copyable.
    RhiFormatCaps (*FormatCapsFn)(RhiFormat Format) = nullptr;

    /// @brief Thin wrapper around the function pointer above.
    RhiFormatCaps formatCaps(RhiFormat Format) const noexcept
    {
        return FormatCapsFn ? FormatCapsFn(Format) : RhiFormatCaps::None;
    }
};

} // namespace goleta::rhi
