#pragma once

/// @file
/// @brief Enumerations shared across the RHI public surface.

#include <cstdint>

namespace goleta::rhi
{

/// @brief Which backend a given instance targets.
enum class BackendKind : uint8_t
{
    Null      = 0,
    D3D12     = 1,
    Vulkan    = 2,
    D3D12Xbox = 3,
    GNMX      = 4,
    NVN       = 5,
};

/// @brief Kind of native queue. Maps to D3D12 command list type and Vulkan queue family.
enum class RhiQueueKind : uint8_t
{
    Graphics = 0,
    Compute  = 1,
    Copy     = 2,
    Video    = 3,
    Count,
};

/// @brief Concrete resource kind exposed via IRhiResource::kind().
enum class RhiResourceKind : uint8_t
{
    Unknown = 0,
    Buffer,
    Texture,
    Sampler,
    ShaderModule,
    GraphicsPipeline,
    ComputePipeline,
    RayTracingPipeline,
    AccelStructure,
    MemoryHeap,
    DescriptorHeap,
    DescriptorSet,
    QueryHeap,
    Fence,
    CommandList,
    CommandPool,
    SwapChain,
    Queue,
};

/// @brief Where a buffer or texture lives and how the CPU can touch it.
enum class RhiMemoryLocation : uint8_t
{
    DeviceLocal = 0, // GPU VRAM; not CPU-mappable on discrete hardware.
    Upload,          // CPU-write, GPU-read; maps to D3D12 UPLOAD / VK HOST_VISIBLE | DEVICE_LOCAL-ish.
    Readback,        // GPU-write, CPU-read.
};

/// @brief Buffer usage flags; bitwise OR. Unused bits are ignored.
enum class RhiBufferUsage : uint32_t
{
    None               = 0,
    VertexBuffer       = 1u << 0,
    IndexBuffer        = 1u << 1,
    ConstantBuffer     = 1u << 2,
    StorageBuffer      = 1u << 3, // SRV+UAV addressable structured / raw.
    IndirectBuffer     = 1u << 4,
    CopySource         = 1u << 5,
    CopyDest           = 1u << 6,
    AccelBuildInput    = 1u << 7,
    AccelStorage       = 1u << 8,
    ShaderBindingTable = 1u << 9,
};

constexpr RhiBufferUsage operator|(RhiBufferUsage A, RhiBufferUsage B) noexcept
{
    return static_cast<RhiBufferUsage>(static_cast<uint32_t>(A) | static_cast<uint32_t>(B));
}
constexpr RhiBufferUsage operator&(RhiBufferUsage A, RhiBufferUsage B) noexcept
{
    return static_cast<RhiBufferUsage>(static_cast<uint32_t>(A) & static_cast<uint32_t>(B));
}
constexpr bool anyOf(RhiBufferUsage Bits, RhiBufferUsage Mask) noexcept
{
    return (static_cast<uint32_t>(Bits) & static_cast<uint32_t>(Mask)) != 0;
}

/// @brief Texture usage flags; bitwise OR.
enum class RhiTextureUsage : uint32_t
{
    None                  = 0,
    Sampled               = 1u << 0,
    Storage               = 1u << 1,
    ColorAttachment       = 1u << 2,
    DepthAttachment       = 1u << 3,
    CopySource            = 1u << 4,
    CopyDest              = 1u << 5,
    ShadingRateAttachment = 1u << 6,
};

constexpr RhiTextureUsage operator|(RhiTextureUsage A, RhiTextureUsage B) noexcept
{
    return static_cast<RhiTextureUsage>(static_cast<uint32_t>(A) | static_cast<uint32_t>(B));
}
constexpr RhiTextureUsage operator&(RhiTextureUsage A, RhiTextureUsage B) noexcept
{
    return static_cast<RhiTextureUsage>(static_cast<uint32_t>(A) & static_cast<uint32_t>(B));
}
constexpr bool anyOf(RhiTextureUsage Bits, RhiTextureUsage Mask) noexcept
{
    return (static_cast<uint32_t>(Bits) & static_cast<uint32_t>(Mask)) != 0;
}

enum class RhiTextureDimension : uint8_t
{
    Tex1D = 0,
    Tex2D,
    Tex3D,
    TexCube,
};

enum class RhiSampleCount : uint8_t
{
    X1  = 1,
    X2  = 2,
    X4  = 4,
    X8  = 8,
    X16 = 16,
};

enum class RhiIndexType : uint8_t
{
    Uint16 = 0,
    Uint32 = 1,
};

enum class RhiPrimitiveTopology : uint8_t
{
    PointList = 0,
    LineList,
    LineStrip,
    TriangleList,
    TriangleStrip,
    PatchList,
};

enum class RhiCompareOp : uint8_t
{
    Never = 0,
    Less,
    Equal,
    LessEqual,
    Greater,
    NotEqual,
    GreaterEqual,
    Always,
};

enum class RhiFilter : uint8_t
{
    Nearest = 0,
    Linear,
};

enum class RhiSamplerAddressMode : uint8_t
{
    Repeat = 0,
    MirrorRepeat,
    ClampToEdge,
    ClampToBorder,
};

enum class RhiBorderColor : uint8_t
{
    TransparentBlack = 0,
    OpaqueBlack,
    OpaqueWhite,
};

enum class RhiLoadOp : uint8_t
{
    Load = 0,
    Clear,
    DontCare
};
enum class RhiStoreOp : uint8_t
{
    Store = 0,
    DontCare,
    Resolve
};

enum class RhiShaderStage : uint8_t
{
    Vertex = 0,
    Pixel,
    Geometry,
    Hull,
    Domain,
    Compute,
    Amplification,
    Mesh,
    RayGen,
    Miss,
    ClosestHit,
    AnyHit,
    Intersection,
    Callable,
    Count,
};

/// @brief Bitmask over RhiShaderStage (one bit per stage).
enum class RhiShaderStageMask : uint32_t
{
    None          = 0,
    Vertex        = 1u << 0,
    Pixel         = 1u << 1,
    Geometry      = 1u << 2,
    Hull          = 1u << 3,
    Domain        = 1u << 4,
    Compute       = 1u << 5,
    Amplification = 1u << 6,
    Mesh          = 1u << 7,
    AllGraphics   = Vertex | Pixel | Geometry | Hull | Domain | Amplification | Mesh,
    AllRayTracing = 1u << 8, // any RT stage
    All           = 0xFFFFFFFF,
};

constexpr RhiShaderStageMask operator|(RhiShaderStageMask A, RhiShaderStageMask B) noexcept
{
    return static_cast<RhiShaderStageMask>(static_cast<uint32_t>(A) | static_cast<uint32_t>(B));
}
constexpr RhiShaderStageMask operator&(RhiShaderStageMask A, RhiShaderStageMask B) noexcept
{
    return static_cast<RhiShaderStageMask>(static_cast<uint32_t>(A) & static_cast<uint32_t>(B));
}
constexpr bool anyOf(RhiShaderStageMask Bits, RhiShaderStageMask Mask) noexcept
{
    return (static_cast<uint32_t>(Bits) & static_cast<uint32_t>(Mask)) != 0;
}

enum class RhiShaderKind : uint8_t
{
    DXIL = 0,  // Precompiled DXIL blob (D3D12 / Xbox).
    SPIRV,     // Precompiled SPIR-V blob (Vulkan).
    PSSL,      // Platform-private (PS5).
    NVNShader, // Platform-private (Switch).
};

/// @brief Failure reasons surfaced through Result<T, RhiError>. Success is represented by the
///        Ok variant and has no member here.
enum class RhiError : uint32_t
{
    Unknown         = 1,
    Unsupported     = 2,
    InvalidArgument = 3,
    OutOfMemory     = 4,
    DeviceLost      = 5,
    NotFound        = 6,
    Timeout         = 7,
    OutOfDate       = 8, ///< Swapchain no longer matches the surface; caller must recreate.
    Suboptimal      = 9, ///< Swapchain still usable but should be recreated when convenient.
};

/// @brief Return status for timeline-fence wait. Ok(Reached) / Ok(TimedOut) are both normal
///        outcomes; only real failures (device lost) come back as Err.
enum class RhiWaitStatus : uint8_t
{
    Reached  = 0,
    TimedOut = 1,
};

} // namespace goleta::rhi
