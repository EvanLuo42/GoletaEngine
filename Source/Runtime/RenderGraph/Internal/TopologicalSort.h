#pragma once

/// @file
/// @brief Kahn's algorithm for topological ordering of the pass DAG.

#include <cstdint>
#include <span>
#include <vector>

namespace goleta::rg::detail
{

/// @brief Topologically sort nodes [0, NodeCount) given directed edges. Returns false if a
///        cycle is detected; the output vector is partial / undefined in that case.
/// @param NodeCount Total number of nodes (pass indices).
/// @param EdgesFrom Parallel array with EdgesTo; edge i goes from EdgesFrom[i] to EdgesTo[i].
/// @param EdgesTo   Parallel array with EdgesFrom.
/// @param Out       Filled with a valid topological ordering of NodeCount entries on success.
[[nodiscard]] bool topologicalSort(uint32_t NodeCount,
                                   std::span<const uint32_t> EdgesFrom,
                                   std::span<const uint32_t> EdgesTo,
                                   std::vector<uint32_t>&    Out);

} // namespace goleta::rg::detail
