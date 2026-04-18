/// @file
/// @brief Engine lifecycle implementation: dependency-ordered init, staged tick.

#include "EngineCore.h"

#include <algorithm>
#include <cassert>
#include <queue>
#include <ranges>

#include "SubsystemRegistry.h"

namespace goleta
{

namespace
{

/// @brief Kahn's algorithm over accepted registry entries. Returns indices into Accepted in
///        dependency-topological order (dependencies before dependents). Within an equal
///        topological rank, registration order is preserved (FIFO queue).
/// @note  Asserts if any declared dependency is missing from Accepted, or if a cycle exists.
Vec<size_t> topoSortAccepted(const Vec<const detail::SubsystemFactoryEntry*>& Accepted)
{
    const size_t N = Accepted.len();

    HashMap<detail::SubsystemTypeId, size_t> IndexOf;
    IndexOf.reserve(N);
    for (size_t I = 0; I < N; ++I)
    {
        IndexOf.insert(Accepted[I]->TypeId, I);
    }

    Vec<Vec<size_t>> Forward(N);
    Vec<size_t> InDegree(N, 0);

    for (size_t I = 0; I < N; ++I)
    {
        for (const detail::SubsystemTypeId& Dep : Accepted[I]->Dependencies)
        {
            const size_t* DepIndex = IndexOf.get(Dep);
            assert(DepIndex && "Subsystem declares a dependency that is not registered for this Engine's categories");
            if (!DepIndex)
            {
                continue;
            }
            Forward[*DepIndex].push(I);
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

    Vec<size_t> Sorted;
    Sorted.reserve(N);
    while (!Ready.empty())
    {
        const size_t I = Ready.front();
        Ready.pop();
        Sorted.push(I);
        for (size_t Next : Forward[I])
        {
            if (--InDegree[Next] == 0)
            {
                Ready.push(Next);
            }
        }
    }

    assert(Sorted.len() == N && "Subsystem dependency cycle detected");
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

    Vec<const detail::SubsystemFactoryEntry*> Accepted;
    Accepted.reserve(Registry.len());
    for (const auto& Entry : Registry)
    {
        if (acceptsCategory(Entry.Category))
        {
            Accepted.push(&Entry);
        }
    }

    const Vec<size_t> Order = topoSortAccepted(Accepted);

    InitOrder.reserve(Order.len());

    for (const size_t Idx : Order)
    {
        const auto& Entry = *Accepted[Idx];
        if (Subsystems.containsKey(Entry.TypeId))
        {
            continue;
        }

        Box<Subsystem> Instance = Entry.Factory();
        Subsystem* Raw = Instance.get();
        Subsystems.insert(Entry.TypeId, std::move(Instance));
        InitOrder.push(Raw);
    }

    for (Subsystem* S : InitOrder)
    {
        S->initialize(*this);
        if (S->shouldTick())
        {
            TickOrder.push(S);
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
