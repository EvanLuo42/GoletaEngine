#pragma once

/// @file
/// @brief Recording schedule: a list of pass groups, one command list per group. Groups on
///        different queues synchronise via per-group signal values on the graph's timeline
///        fence.

#include <cstdint>
#include <vector>

#include "RHIEnums.h"

namespace goleta::rg
{

struct ScheduleGroup
{
    /// @brief Indices into CompiledGraph::PassOrder.
    std::vector<uint32_t> Passes;

    /// @brief Queue the group's command list will be submitted on.
    rhi::RhiQueueKind QueueKind = rhi::RhiQueueKind::Graphics;

    /// @brief Indices (into ExecuteSchedule::Groups) of prior groups whose completion this
    ///        group must wait on before it can start. Populated only for cross-queue edges;
    ///        same-queue ordering is implicit from submission order.
    std::vector<uint32_t> WaitGroups;

    /// @brief Offset of this group's signal relative to the start of the frame's fence
    ///        window (1-based). The absolute signal value used at submit is
    ///        (graph frame base) + SignalOffset.
    uint64_t SignalOffset = 0;

    /// @brief Logical resources whose first use falls inside this group and therefore must be
    ///        acquired from the pool before recording. Indexed into CompiledGraph::LogicalResources.
    std::vector<uint32_t> AcquireAtStart;

    /// @brief Logical resources whose last use falls inside this group. Released to the pool
    ///        after the group has been recorded so subsequent same-desc logicals can reuse them
    ///        within the same frame.
    std::vector<uint32_t> ReleaseAfter;
};

struct ExecuteSchedule
{
    std::vector<ScheduleGroup> Groups;

    /// @brief Total signal values this schedule consumes per frame (== Groups.size()).
    uint64_t FenceValuesPerFrame = 0;
};

} // namespace goleta::rg
