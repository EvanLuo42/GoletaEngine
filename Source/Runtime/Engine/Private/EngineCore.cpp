/// @file
/// @brief Engine lifecycle implementation: dependency-ordered init, staged tick.

#include "EngineCore.h"

#include <algorithm>
#include <cassert>
#include <queue>
#include <ranges>
#include <unordered_map>
#include <vector>

#include "SubsystemRegistry.h"

namespace goleta
{

namespace
{

/// @brief Kahn's algorithm over accepted registry entries. Returns indices into Accepted in
///        dependency-topological order (dependencies before dependents). Within an equal
///        topological rank, registration order is preserved (FIFO queue).
/// @note  Asserts if any declared dependency is missing from Accepted, or if a cycle exists.
std::vector<size_t> topoSortAccepted(const std::vector<const detail::SubsystemFactoryEntry*>& Accepted)
{
    const size_t N = Accepted.size();

    std::unordered_map<detail::SubsystemTypeId, size_t> IndexOf;
    IndexOf.reserve(N);
    for (size_t I = 0; I < N; ++I)
    {
        IndexOf.emplace(Accepted[I]->TypeId, I);
    }

    std::vector<std::vector<size_t>> Forward(N);
    std::vector<size_t> InDegree(N, 0);

    for (size_t I = 0; I < N; ++I)
    {
        for (const detail::SubsystemTypeId& Dep : Accepted[I]->Dependencies)
        {
            auto DepIt = IndexOf.find(Dep);
            assert(DepIt != IndexOf.end() &&
                   "Subsystem declares a dependency that is not registered for this Engine's categories");
            if (DepIt == IndexOf.end())
            {
                continue;
            }
            Forward[DepIt->second].push_back(I);
            ++InDegree[I];
        }
    }

    std::queue<size_t> Ready;
    for (size_t I = 0; I < N; ++I)
    {
        if (InDegree[I] == 0)
        {
            Ready.push(I);
        }
    }

    std::vector<size_t> Sorted;
    Sorted.reserve(N);
    while (!Ready.empty())
    {
        const size_t I = Ready.front();
        Ready.pop();
        Sorted.push_back(I);
        for (size_t Next : Forward[I])
        {
            if (--InDegree[Next] == 0)
            {
                Ready.push(Next);
            }
        }
    }

    assert(Sorted.size() == N && "Subsystem dependency cycle detected");
    return Sorted;
}

} // namespace

Engine::Engine() = default;

Engine::~Engine()
{
    if (Running)
    {
        stop();
    }
}

bool Engine::acceptsCategory(SubsystemCategory Category) const
{
    return Category == SubsystemCategory::Engine || Category == SubsystemCategory::Game;
}

void Engine::start()
{
    if (Running)
    {
        return;
    }

    const auto& Registry = detail::subsystemRegistry();

    std::vector<const detail::SubsystemFactoryEntry*> Accepted;
    Accepted.reserve(Registry.size());
    for (const auto& Entry : Registry)
    {
        if (acceptsCategory(Entry.Category))
        {
            Accepted.push_back(&Entry);
        }
    }

    const std::vector<size_t> Order = topoSortAccepted(Accepted);

    InitOrder.reserve(Order.size());

    for (const size_t Idx : Order)
    {
        const auto& Entry = *Accepted[Idx];
        if (Subsystems.contains(Entry.TypeId))
        {
            continue;
        }

        std::unique_ptr<Subsystem> Instance = Entry.Factory();
        Subsystem* Raw = Instance.get();
        Subsystems.emplace(Entry.TypeId, std::move(Instance));
        InitOrder.push_back(Raw);
    }

    for (Subsystem* S : InitOrder)
    {
        S->initialize(*this);
        if (S->shouldTick())
        {
            TickOrder.push_back(S);
        }
    }

    // stable_sort preserves dependency order within the same numeric stage.
    std::ranges::stable_sort(TickOrder, [](const Subsystem* L, const Subsystem* R)
                             { return static_cast<uint16_t>(L->tickStage()) < static_cast<uint16_t>(R->tickStage()); });

    Running = true;
}

void Engine::tick(const float DeltaSeconds)
{
    if (!Running)
    {
        return;
    }
    for (Subsystem* S : TickOrder)
    {
        S->tick(DeltaSeconds);
    }
}

void Engine::stop()
{
    if (!Running)
    {
        return;
    }

    for (auto& It : std::views::reverse(InitOrder))
    {
        It->deinitialize();
    }

    TickOrder.clear();
    InitOrder.clear();
    Subsystems.clear();
    Running = false;
}

} // namespace goleta
