#include "TopologicalSort.h"

#include <queue>

namespace goleta::rg::detail
{

bool topologicalSort(uint32_t NodeCount, std::span<const uint32_t> EdgesFrom,
                     std::span<const uint32_t> EdgesTo, std::vector<uint32_t>& Out)
{
    Out.clear();
    Out.reserve(NodeCount);

    std::vector<uint32_t> InDegree(NodeCount, 0);
    std::vector<std::vector<uint32_t>> Adj(NodeCount);
    for (size_t i = 0; i < EdgesFrom.size(); ++i)
    {
        const uint32_t From = EdgesFrom[i];
        const uint32_t To   = EdgesTo[i];
        if (From >= NodeCount || To >= NodeCount)
            return false;
        Adj[From].push_back(To);
        ++InDegree[To];
    }

    // Seed the queue in ascending node order so the sort is stable (matches registration order
    // within a topological rank).
    std::queue<uint32_t> Ready;
    for (uint32_t n = 0; n < NodeCount; ++n)
        if (InDegree[n] == 0)
            Ready.push(n);

    while (!Ready.empty())
    {
        const uint32_t n = Ready.front();
        Ready.pop();
        Out.push_back(n);
        for (const uint32_t m : Adj[n])
        {
            if (--InDegree[m] == 0)
                Ready.push(m);
        }
    }

    return Out.size() == NodeCount;
}

} // namespace goleta::rg::detail
