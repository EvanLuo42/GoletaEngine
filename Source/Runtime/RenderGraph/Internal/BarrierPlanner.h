#pragma once

/// @file
/// @brief Compiles per-pass RhiBarrierGroups from the topologically-sorted pass list and
///        each field's declared access mode + bind.

#include <cstdint>
#include <vector>

#include "CompiledGraph.h"
#include "RHIBarrier.h"
#include "RHIEnums.h"
#include "RenderGraphTypes.h"

namespace goleta::rg
{

class RenderPassReflection;

/// @brief Planner input: one entry per pass, in execution order.
struct PlannerPassView
{
    PassId                      Id;
    const RenderPassReflection* Reflection = nullptr;
    rhi::RhiQueueKind           QueueKind  = rhi::RhiQueueKind::Graphics;
};

/// @brief Per-logical-resource terminal target specified by RenderGraph::markOutput().
struct PlannerMarkedOutput
{
    LogicalResourceId     Resource = LogicalResourceId::Invalid;
    rhi::RhiTextureLayout Terminal = rhi::RhiTextureLayout::ShaderResource;
};

/// @brief Plan per-pass pre-barriers and final-barriers for the compiled graph.
/// @param Passes              Topologically-ordered passes.
/// @param LogicalResources    Merged descriptors for each logical resource.
/// @param FieldToLogical      Indexed by [PassId][FieldId].
/// @param MarkedOutputs       Graph-level terminals.
/// @param FinalQueue          Queue kind on which the final barriers will be recorded; stage
///                            masks restrict sync bits to that queue's supported set.
/// @param OutPrePassBarriers  Output; sized to Passes.size().
/// @param OutFinalBarriers    Output; appended with terminal transitions.
void planBarriers(std::span<const PlannerPassView>                     Passes,
                  std::span<const LogicalResourceInfo>                 LogicalResources,
                  const std::vector<std::vector<LogicalResourceId>>&   FieldToLogical,
                  std::span<const PlannerMarkedOutput>                 MarkedOutputs,
                  rhi::RhiQueueKind                                    FinalQueue,
                  std::vector<PassBarriers>&                           OutPrePassBarriers,
                  PassBarriers&                                        OutFinalBarriers);

} // namespace goleta::rg
