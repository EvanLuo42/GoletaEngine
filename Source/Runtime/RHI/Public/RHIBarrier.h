#pragma once

/// @file
/// @brief Enhanced barrier API. Maps 1:1 onto D3D12 BarrierGroup / Vulkan sync2. No legacy
///        ResourceBarrier flavor is exposed.

#include <cstdint>

#include "RHIEnums.h"
#include "RHIExport.h"
#include "RHIStructChain.h"

namespace goleta::rhi
{

class IRhiBuffer;
class IRhiTexture;

/// @brief Pipeline stages for sync. Bitmask; OR freely.
enum class RhiPipelineStage : uint32_t
{
    None               = 0,
    DrawIndirect       = 1u << 0,
    VertexInput        = 1u << 1,
    VertexShader       = 1u << 2,
    PixelShader        = 1u << 3,
    EarlyFragmentTests = 1u << 4,
    LateFragmentTests  = 1u << 5,
    ColorAttachmentOut = 1u << 6,
    ComputeShader      = 1u << 7,
    RayTracingShader   = 1u << 8,
    Copy               = 1u << 9,
    Resolve            = 1u << 10,
    AccelerationBuild  = 1u << 11,
    AllGraphics        = 1u << 12,
    AllCommands        = 1u << 13,
};

constexpr RhiPipelineStage operator|(RhiPipelineStage A, RhiPipelineStage B) noexcept
{
    return static_cast<RhiPipelineStage>(static_cast<uint32_t>(A) | static_cast<uint32_t>(B));
}
constexpr RhiPipelineStage operator&(RhiPipelineStage A, RhiPipelineStage B) noexcept
{
    return static_cast<RhiPipelineStage>(static_cast<uint32_t>(A) & static_cast<uint32_t>(B));
}

/// @brief Access flags describing how a resource is touched. Bitmask.
enum class RhiAccess : uint32_t
{
    None                 = 0,
    IndirectRead         = 1u << 0,
    IndexRead            = 1u << 1,
    VertexRead           = 1u << 2,
    ConstantBufferRead   = 1u << 3,
    ShaderResourceRead   = 1u << 4,
    UnorderedAccessRead  = 1u << 5,
    UnorderedAccessWrite = 1u << 6,
    ColorAttachmentRead  = 1u << 7,
    ColorAttachmentWrite = 1u << 8,
    DepthRead            = 1u << 9,
    DepthWrite           = 1u << 10,
    CopyRead             = 1u << 11,
    CopyWrite            = 1u << 12,
    AccelRead            = 1u << 13,
    AccelWrite           = 1u << 14,
    Present              = 1u << 15,
};

constexpr RhiAccess operator|(RhiAccess A, RhiAccess B) noexcept
{
    return static_cast<RhiAccess>(static_cast<uint32_t>(A) | static_cast<uint32_t>(B));
}
constexpr RhiAccess operator&(RhiAccess A, RhiAccess B) noexcept
{
    return static_cast<RhiAccess>(static_cast<uint32_t>(A) & static_cast<uint32_t>(B));
}

/// @brief Enhanced-barrier texture layout. Names match D3D12 enhanced-barrier layouts.
enum class RhiTextureLayout : uint8_t
{
    Undefined = 0,
    Common,
    GenericRead,
    ShaderResource,
    UnorderedAccess,
    ColorAttachment,
    DepthStencilWrite,
    DepthStencilRead,
    CopySource,
    CopyDest,
    ResolveSource,
    ResolveDest,
    Present,
};

struct RhiGlobalBarrier
{
    RhiPipelineStage SrcStages = RhiPipelineStage::None;
    RhiPipelineStage DstStages = RhiPipelineStage::None;
    RhiAccess        SrcAccess = RhiAccess::None;
    RhiAccess        DstAccess = RhiAccess::None;
};

struct RhiBufferBarrier
{
    IRhiBuffer*      Buffer    = nullptr;
    RhiPipelineStage SrcStages = RhiPipelineStage::None;
    RhiPipelineStage DstStages = RhiPipelineStage::None;
    RhiAccess        SrcAccess = RhiAccess::None;
    RhiAccess        DstAccess = RhiAccess::None;
    uint64_t         Offset    = 0;
    uint64_t         Size      = ~uint64_t{0}; // VK_WHOLE_SIZE semantics.
};

struct RhiTextureBarrier
{
    IRhiTexture*     Texture         = nullptr;
    RhiPipelineStage SrcStages       = RhiPipelineStage::None;
    RhiPipelineStage DstStages       = RhiPipelineStage::None;
    RhiAccess        SrcAccess       = RhiAccess::None;
    RhiAccess        DstAccess       = RhiAccess::None;
    RhiTextureLayout SrcLayout       = RhiTextureLayout::Undefined;
    RhiTextureLayout DstLayout       = RhiTextureLayout::Common;
    uint32_t         BaseMipLevel    = 0;
    uint32_t         MipLevelCount   = 0; // 0 == all.
    uint32_t         BaseArrayLayer  = 0;
    uint32_t         ArrayLayerCount = 0; // 0 == all.
    bool             DiscardContents = false;
};

/// @brief Batched barrier group; submitted through IRhiCommandList::barriers().
struct RhiBarrierGroup
{
    static constexpr RhiStructType kStructType = RhiStructType::BarrierGroup;
    RhiStructHeader                Header{kStructType, nullptr};

    const RhiGlobalBarrier*  Globals      = nullptr;
    uint32_t                 GlobalCount  = 0;
    const RhiBufferBarrier*  Buffers      = nullptr;
    uint32_t                 BufferCount  = 0;
    const RhiTextureBarrier* Textures     = nullptr;
    uint32_t                 TextureCount = 0;
};

} // namespace goleta::rhi
