#pragma once

/// @file
/// @brief Private Impl struct shared by RenderGraph.cpp, RenderGraphCompile.cpp, and
///        RenderGraphExecute.cpp.

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "CommandPoolCache.h"
#include "CompiledGraph.h"
#include "ExecuteSchedule.h"
#include "Memory/Rc.h"
#include "RHIBarrier.h"
#include "RHIBuffer.h"
#include "RHIQuery.h"
#include "RHISync.h"
#include "RenderGraph.h"
#include "RenderGraphTypes.h"
#include "RenderPass.h"
#include "RenderPassReflection.h"
#include "ResourcePool.h"
#include "ResourceRegistry.h"

namespace enki
{
class TaskScheduler;
}

namespace goleta::rhi
{
class IRhiTexture;
class IRhiBuffer;
} // namespace goleta::rhi

namespace goleta::rg
{

struct GraphEdge
{
    PassId  SrcPass;
    FieldId SrcField;
    PassId  DstPass;
    FieldId DstField;
};

struct ImportedBinding
{
    std::string           PassName;
    std::string           FieldName;
    Rc<rhi::IRhiTexture>  Texture;
    Rc<rhi::IRhiBuffer>   Buffer;
    rhi::RhiTextureLayout InitialLayout = rhi::RhiTextureLayout::Undefined;
    rhi::RhiAccess        InitialAccess = rhi::RhiAccess::None;
};

struct MarkedOutputRequest
{
    PassId                Pass;
    FieldId               Field;
    rhi::RhiTextureLayout Terminal;
};

struct RenderGraph::Impl
{
    std::string                             DebugName;
    std::vector<Rc<IRenderPass>>            Passes;
    std::vector<std::string>                PassNames;
    std::vector<std::unique_ptr<RenderPassReflection>> Reflections;
    std::unordered_map<std::string, PassId> PassByName;

    std::vector<GraphEdge>          Edges;
    std::vector<ImportedBinding>    Imports;
    std::vector<MarkedOutputRequest> MarkedOutputs;

    CompiledGraph     Compiled;
    ExecuteSchedule   Schedule;
    ResourcePool      Pool;
    ResourceRegistry  Registry;
    CommandPoolCache  CommandPools;

    Rc<rhi::IRhiFence>   TimelineFence;
    uint64_t             FenceFrameBase = 0;

    enki::TaskScheduler* TaskScheduler = nullptr;

    bool        CaptureRequested = false;
    std::string CaptureName;

    bool                              TimingsEnabled = false;
    Rc<rhi::IRhiQueryHeap>            TimestampHeap;
    Rc<rhi::IRhiBuffer>               TimestampReadback;
    uint32_t                          TimestampCapacity = 0;
    uint64_t                          PendingResolveFenceValue = 0;
    std::vector<RenderGraph::PassTiming> TimingsLastFrame;
    std::vector<RenderGraph::PassTiming> TimingsInFlight;

    bool Dirty     = true;
    bool Compiled_ = false;
};

} // namespace goleta::rg
