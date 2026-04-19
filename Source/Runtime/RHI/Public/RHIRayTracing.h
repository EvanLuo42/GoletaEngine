#pragma once

/// @file
/// @brief Ray-tracing interfaces. Always declared; gated at runtime via
///        IRhiDevice::features().RayTracing.Supported.

#include <cstdint>

#include "RHIEnums.h"
#include "RHIExport.h"
#include "RHIPipeline.h"
#include "RHIResource.h"
#include "RHIStructChain.h"

namespace goleta::rhi
{

class IRhiBuffer;
class IRhiShaderModule;

enum class RhiAccelStructureKind : uint8_t
{
    BottomLevel = 0,
    TopLevel,
};

enum class RhiAccelGeometryKind : uint8_t
{
    Triangles = 0,
    Aabbs,
    Instances,
};

enum class RhiAccelBuildFlags : uint32_t
{
    None            = 0,
    AllowUpdate     = 1u << 0,
    AllowCompaction = 1u << 1,
    PreferFastTrace = 1u << 2,
    PreferFastBuild = 1u << 3,
    MinimizeMemory  = 1u << 4,
};

constexpr RhiAccelBuildFlags operator|(RhiAccelBuildFlags A, RhiAccelBuildFlags B) noexcept
{
    return static_cast<RhiAccelBuildFlags>(static_cast<uint32_t>(A) | static_cast<uint32_t>(B));
}

struct RhiAccelTriangleGeometry
{
    IRhiBuffer* VertexBuffer = nullptr;
    uint64_t    VertexOffset = 0;
    uint32_t    VertexStride = 0;
    uint32_t    VertexCount  = 0;
    RhiFormat   VertexFormat = RhiFormat::Rgb32Float;

    IRhiBuffer*  IndexBuffer = nullptr;
    uint64_t     IndexOffset = 0;
    uint32_t     IndexCount  = 0;
    RhiIndexType IndexType   = RhiIndexType::Uint32;

    IRhiBuffer* TransformBuffer = nullptr; // Nullable; 3x4 affine rows.
    uint64_t    TransformOffset = 0;
};

struct RhiAccelAabbGeometry
{
    IRhiBuffer* AabbBuffer = nullptr;
    uint64_t    AabbOffset = 0;
    uint32_t    AabbStride = 24;
    uint32_t    AabbCount  = 0;
};

struct RhiAccelGeometry
{
    RhiAccelGeometryKind     Kind   = RhiAccelGeometryKind::Triangles;
    bool                     Opaque = true;
    RhiAccelTriangleGeometry Triangles{};
    RhiAccelAabbGeometry     Aabbs{};
};

struct RhiAccelStructureDesc
{
    static constexpr RhiStructType kStructType = RhiStructType::AccelStructureDesc;
    RhiStructHeader                Header{kStructType, nullptr};

    RhiAccelStructureKind Kind  = RhiAccelStructureKind::BottomLevel;
    RhiAccelBuildFlags    Flags = RhiAccelBuildFlags::PreferFastTrace;

    // For BottomLevel:
    const RhiAccelGeometry* Geometries    = nullptr;
    uint32_t                GeometryCount = 0;

    // For TopLevel:
    uint32_t InstanceCount = 0;

    const char* DebugName = nullptr;
};

struct RhiAccelBuildDesc
{
    class IRhiAccelStructure* Dest          = nullptr;
    class IRhiAccelStructure* Source        = nullptr; // Non-null for update builds.
    IRhiBuffer*               ScratchBuffer = nullptr;
    uint64_t                  ScratchOffset = 0;

    /// @brief TopLevel only: buffer of VkAccelerationStructureInstanceKHR / D3D12_RAYTRACING_INSTANCE_DESC.
    IRhiBuffer* InstanceBuffer = nullptr;
    uint64_t    InstanceOffset = 0;
};

class RHI_API IRhiAccelStructure : public IRhiResource
{
public:
    virtual const RhiAccelStructureDesc& desc() const noexcept                              = 0;
    virtual uint64_t                     gpuAddress() const noexcept                        = 0;
    virtual uint64_t                     requiredScratchSize(bool ForUpdate) const noexcept = 0;
    virtual uint64_t                     resultBufferSize() const noexcept                  = 0;
};

struct RhiRayTracingShaderGroup
{
    enum class Kind : uint8_t
    {
        General = 0,
        TrianglesHit,
        ProceduralHit
    };
    Kind              Type               = Kind::General;
    IRhiShaderModule* GeneralShader      = nullptr;
    IRhiShaderModule* ClosestHitShader   = nullptr;
    IRhiShaderModule* AnyHitShader       = nullptr;
    IRhiShaderModule* IntersectionShader = nullptr;
    const char*       Name               = nullptr;
};

struct RhiRayTracingPipelineDesc
{
    static constexpr auto kStructType = RhiStructType::RayTracingPipelineDesc;
    RhiStructHeader       Header{kStructType, nullptr};

    const RhiRayTracingShaderGroup* Groups     = nullptr;
    uint32_t                        GroupCount = 0;

    RhiBindingLayout Bindings{};
    uint32_t         MaxRecursionDepth = 1;
    uint32_t         MaxPayloadBytes   = 16;
    uint32_t         MaxAttributeBytes = 8;

    const char* DebugName = nullptr;
};

class RHI_API IRhiRayTracingPipeline : public IRhiResource
{
public:
    virtual const RhiRayTracingPipelineDesc& desc() const noexcept = 0;

    /// @brief Retrieve opaque shader group handles for building the shader binding table.
    ///        The backend writes HandleSize bytes per group into OutBytes.
    virtual uint32_t shaderGroupHandleSize() const noexcept                                                        = 0;
    virtual uint32_t shaderGroupHandleAlignment() const noexcept                                                   = 0;
    virtual void getShaderGroupHandles(uint32_t FirstGroup, uint32_t GroupCount, void* OutBytes, uint32_t OutSize) = 0;
};

} // namespace goleta::rhi
