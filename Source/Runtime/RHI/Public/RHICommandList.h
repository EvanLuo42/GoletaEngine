#pragma once

/// @file
/// @brief Command list recording interface. Recording is single-threaded per command list;
///        each worker thread owns its own list. Submission is via IRhiQueue::submit.

#include <cstdint>

#include "RHIBarrier.h"
#include "RHIEnums.h"
#include "RHIExport.h"
#include "RHIResource.h"
#include "RHIStructChain.h"

namespace goleta::rhi
{

class IRhiBuffer;
class IRhiTexture;
class IRhiTextureView;
class IRhiGraphicsPipeline;
class IRhiComputePipeline;
class IRhiQueryHeap;
class IRhiDescriptorSet;
class IRhiCommandList;

struct RhiViewport
{
    float X = 0.0f, Y = 0.0f;
    float Width = 0.0f, Height = 0.0f;
    float MinDepth = 0.0f, MaxDepth = 1.0f;
};

struct RhiRect
{
    int32_t  X = 0, Y = 0;
    uint32_t Width = 0, Height = 0;
};

/// @brief One color / depth-stencil attachment in a dynamic rendering pass.
struct RhiRenderingAttachment
{
    IRhiTextureView* View          = nullptr;
    IRhiTextureView* ResolveView   = nullptr; ///< Nullable.
    RhiLoadOp        LoadOp        = RhiLoadOp::Load;
    RhiStoreOp       StoreOp       = RhiStoreOp::Store;
    float            ClearColor[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    float            ClearDepth    = 1.0f;
    uint32_t         ClearStencil  = 0;
};

struct RhiRenderingDesc
{
    static constexpr auto kStructType = RhiStructType::RenderingDesc;
    RhiStructHeader       Header{kStructType, nullptr};

    RhiRect  RenderArea{};
    uint32_t LayerCount = 1;
    uint32_t ViewMask   = 0; ///< Multiview mask; 0 = disabled.

    const RhiRenderingAttachment* ColorAttachments     = nullptr;
    uint32_t                      ColorAttachmentCount = 0;
    const RhiRenderingAttachment* DepthAttachment      = nullptr;
    const RhiRenderingAttachment* StencilAttachment    = nullptr;
};

/// @brief One region of a texture-to-texture copy.
struct RhiTextureCopy
{
    IRhiTexture* DstTexture    = nullptr;
    uint32_t     DstMipLevel   = 0;
    uint32_t     DstArrayLayer = 0;
    int32_t      DstX = 0, DstY = 0, DstZ = 0;

    IRhiTexture* SrcTexture    = nullptr;
    uint32_t     SrcMipLevel   = 0;
    uint32_t     SrcArrayLayer = 0;
    int32_t      SrcX = 0, SrcY = 0, SrcZ = 0;

    uint32_t Width = 0, Height = 0, Depth = 1;
};

/// @brief Parameters for IRhiCommandList::dispatchRays().
struct RhiShaderBindingTableRange
{
    IRhiBuffer* Buffer      = nullptr;
    uint64_t    Offset      = 0;
    uint64_t    SizeBytes   = 0;
    uint32_t    StrideBytes = 0;
};

struct RhiDispatchRaysDesc
{
    RhiShaderBindingTableRange RayGen;
    RhiShaderBindingTableRange Miss;
    RhiShaderBindingTableRange HitGroup;
    RhiShaderBindingTableRange Callable;
    uint32_t                   Width  = 1;
    uint32_t                   Height = 1;
    uint32_t                   Depth  = 1;
};

struct RhiAccelBuildDesc; // Declared in RHIRayTracing.h.

/// @brief Per-thread command-list allocator. One pool per recording thread per queue kind.
/// @note  A pool is the common abstraction over D3D12 `ID3D12CommandAllocator`, Vulkan
///        `VkCommandPool`, AGC / GNMX user-owned command-buffer memory, and NVN
///        `nvnCommandBufferAddCommandMemory` — all of which require thread-local memory
///        ownership with a batched-reset lifetime model.
///
///        Contracts:
///        - Not thread-safe. A single pool may only be touched by one recording thread at a
///          time. Use one pool per worker thread.
///        - `reset()` recycles every list previously returned by `allocate()`. The caller
///          guarantees the GPU has finished executing those lists (typically by waiting on
///          the fence signaled after their submit).
///        - Lists returned by `allocate()` stay valid until the next `reset()` on this pool
///          or until the pool itself is destroyed.
class RHI_API IRhiCommandPool : public IRhiResource
{
public:
    /// @brief Queue family this pool's lists will be submitted to.
    [[nodiscard]] virtual RhiQueueKind queueKind() const noexcept = 0;

    /// @brief Allocate a fresh command list bound to this pool's queue kind.
    virtual Rc<IRhiCommandList> allocate() = 0;

    /// @brief Recycle every list allocated from this pool. See class-level contract.
    virtual void reset() = 0;
};

/// @brief Command list; records GPU work for later submit(). Obtained from an IRhiCommandPool.
/// @note  Created bound to a specific queue kind. Methods that don't apply to the kind
///        (e.g. drawIndexed on a copy list) assert in debug, no-op in release.
class RHI_API IRhiCommandList : public IRhiResource
{
public:
    virtual RhiQueueKind queueKind() const noexcept = 0;

    virtual void begin() = 0;
    virtual void end()   = 0;
    virtual void reset() = 0; ///< Recycle the underlying allocator. Safe once the GPU is done with it.

    virtual void beginDebugScope(const char* Name, uint32_t ColorRgba)   = 0;
    virtual void endDebugScope()                                         = 0;
    virtual void insertDebugMarker(const char* Name, uint32_t ColorRgba) = 0;

    virtual void barriers(const RhiBarrierGroup& Group) = 0;

    virtual void beginRendering(const RhiRenderingDesc& Desc) = 0;
    virtual void endRendering()                               = 0;

    virtual void setViewports(const RhiViewport* Viewports, uint32_t Count) = 0;
    virtual void setScissors(const RhiRect* Scissors, uint32_t Count)       = 0;
    virtual void setBlendConstants(const float Rgba[4])                     = 0;
    virtual void setStencilReference(uint32_t Reference)                    = 0;

    virtual void setGraphicsPipeline(IRhiGraphicsPipeline* Pipeline) = 0;
    virtual void setComputePipeline(IRhiComputePipeline* Pipeline)   = 0;

    /// @brief Upload data into the root-constants / push-constants block.
    virtual void setRootConstants(const void* Data, uint32_t SizeBytes, uint32_t OffsetBytes = 0) = 0;

    /// @brief Bind a traditional descriptor set to a specific slot. Renderer code using the
    ///        bindless path typically does not call this.
    virtual void setDescriptorSet(uint32_t SetIndex, IRhiDescriptorSet* Set) = 0;

    virtual void setVertexBuffers(uint32_t FirstBinding, IRhiBuffer* const* Buffers, const uint64_t* Offsets,
                                  uint32_t Count)                                       = 0;
    virtual void setIndexBuffer(IRhiBuffer* Buffer, uint64_t Offset, RhiIndexType Type) = 0;

    virtual void draw(uint32_t VertexCount, uint32_t InstanceCount, uint32_t FirstVertex, uint32_t FirstInstance) = 0;
    virtual void drawIndexed(uint32_t IndexCount, uint32_t InstanceCount, uint32_t FirstIndex, int32_t VertexOffset,
                             uint32_t FirstInstance)                                                              = 0;
    virtual void drawIndirect(IRhiBuffer* Args, uint64_t Offset, uint32_t DrawCount, uint32_t Stride)             = 0;
    virtual void drawIndexedIndirect(IRhiBuffer* Args, uint64_t Offset, uint32_t DrawCount, uint32_t Stride)      = 0;
    virtual void dispatch(uint32_t X, uint32_t Y, uint32_t Z)                                                     = 0;
    virtual void dispatchIndirect(IRhiBuffer* Args, uint64_t Offset)                                              = 0;

    virtual void copyBuffer(IRhiBuffer* Dst, uint64_t DstOffset, IRhiBuffer* Src, uint64_t SrcOffset,
                            uint64_t Size)                      = 0;
    virtual void copyTexture(const RhiTextureCopy& Copy)        = 0;
    virtual void copyBufferToTexture(IRhiBuffer* Src, uint64_t SrcOffset, uint32_t RowPitch,
                                     const RhiTextureCopy& Dst) = 0;

    virtual void writeTimestamp(IRhiQueryHeap* Heap, uint32_t Index) = 0;
    virtual void resolveQueries(IRhiQueryHeap* Heap, uint32_t FirstQuery, uint32_t Count, IRhiBuffer* Dst,
                                uint64_t DstOffset)                  = 0;

    // Optional features. Return false if not supported; caller must check features() first.
    virtual bool dispatchRays(const RhiDispatchRaysDesc& Desc)                                                 = 0;
    virtual bool drawMeshTasks(uint32_t X, uint32_t Y, uint32_t Z)                                             = 0;
    virtual bool drawMeshTasksIndirect(IRhiBuffer* Args, uint64_t Offset, uint32_t DrawCount, uint32_t Stride) = 0;
    virtual bool buildAccelStructure(const RhiAccelBuildDesc& Desc)                                            = 0;
};

} // namespace goleta::rhi
