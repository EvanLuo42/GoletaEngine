#pragma once

/// @file
/// @brief Frozen output of RenderGraph::compile(): pass ordering, per-pass barriers,
///        logical-resource descriptors, and field→logical mapping.

#include <cstdint>
#include <string>
#include <vector>

#include "Memory/Rc.h"
#include "RHIBarrier.h"
#include "RHIBuffer.h"
#include "RHITexture.h"
#include "RenderGraphTypes.h"
#include "RenderPassReflection.h"

namespace goleta::rg
{

/// @brief One planned barrier batch. The parallel TextureIds / BufferIds arrays hold the
///        logical resource each barrier targets; the executor patches Texture/Buffer pointers
///        on the Rhi structs before submission.
struct PassBarriers
{
    std::vector<rhi::RhiTextureBarrier> Textures;
    std::vector<LogicalResourceId>      TextureIds;
    std::vector<rhi::RhiBufferBarrier>  Buffers;
    std::vector<LogicalResourceId>      BufferIds;
};

struct LogicalResourceInfo
{
    RgResourceType ResourceType = RgResourceType::Texture;
    RgLifetime     Lifetime     = RgLifetime::Transient;

    rhi::RhiTextureDesc TextureDesc{};
    rhi::RhiBufferDesc  BufferDesc{};

    Rc<rhi::IRhiTexture> ImportedTexture; ///< Set for Imported textures.
    Rc<rhi::IRhiBuffer>  ImportedBuffer;  ///< Set for Imported buffers.

    rhi::RhiTextureLayout InitialLayout = rhi::RhiTextureLayout::Undefined;
    rhi::RhiAccess        InitialAccess = rhi::RhiAccess::None;

    /// @brief Earliest and latest pass (indices into PassOrder) that touch this resource.
    uint32_t FirstUsePassIdx = 0xFFFFFFFFu;
    uint32_t LastUsePassIdx  = 0;

    /// @brief Human-readable name surfaced through backend debug-name hooks so Nsight / PIX /
    ///        RGP show e.g. "gbuffer.albedo" instead of "Unnamed". Built at compile from the
    ///        first owning field's "<pass>.<field>" pair.
    std::string DebugName;
};

struct CompiledGraph
{
    /// @brief Pass IDs in execution order. PassOrder[i] is the PassId of the i-th pass to run.
    std::vector<PassId> PassOrder;

    /// @brief Per reflected pass: the FieldId→LogicalResourceId table.
    ///        FieldToLogical[PassId-as-index][FieldId-as-index] = LogicalResourceId.
    std::vector<std::vector<LogicalResourceId>> FieldToLogical;

    /// @brief Per logical resource, merged desc + lifetime info.
    std::vector<LogicalResourceInfo> LogicalResources;

    /// @brief Pre-barriers injected before each pass's execute(), indexed by position in PassOrder.
    std::vector<PassBarriers> PrePassBarriers;

    /// @brief Final transitions scheduled after the last pass (markOutput terminals, present).
    PassBarriers FinalBarriers;
};

} // namespace goleta::rg
