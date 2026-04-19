/// @file
/// @brief Process-wide backend registry. Thread-safe registration, first-wins semantics.

#include "RHIBackendRegistry.h"

#include <array>
#include <atomic>
#include <mutex>

namespace goleta::rhi
{
namespace
{

constexpr size_t kMaxBackends = static_cast<size_t>(BackendKind::NVN) + 1;

struct RegistryState
{
    std::mutex                                Mutex;
    std::array<RhiBackendEntry, kMaxBackends> Entries{};
    std::atomic<uint32_t>                     Count{0};
};

RegistryState& state()
{
    static RegistryState S;
    return S;
}

} // namespace

void registerRhiBackend(const RhiBackendEntry& Entry)
{
    const size_t Slot = static_cast<size_t>(Entry.Kind);
    if (Slot >= kMaxBackends)
        return;
    if (!Entry.Factory)
        return;

    auto& [Mutex, Entries, Count] = state();
    std::scoped_lock Lock(Mutex);
    if (Entries[Slot].Factory)
        return; // First registration wins.
    Entries[Slot] = Entry;
    Count.fetch_add(1, std::memory_order_release);
}

RhiBackendFactoryFn findRhiBackend(BackendKind Kind) noexcept
{
    const size_t Slot = static_cast<size_t>(Kind);
    if (Slot >= kMaxBackends)
        return nullptr;

    RegistryState&   S = state();
    std::scoped_lock Lock(S.Mutex);
    return S.Entries[Slot].Factory;
}

namespace detail
{

void resetRhiBackendRegistryForTests()
{
    auto& [Mutex, Entries, Count] = state();
    std::scoped_lock Lock(Mutex);
    for (size_t Slot = 0; Slot < kMaxBackends; ++Slot)
    {
        Entries[Slot] = RhiBackendEntry{};
    }
    Count.store(0, std::memory_order_release);
}

uint32_t rhiBackendRegistrationCount() noexcept { return state().Count.load(std::memory_order_acquire); }

} // namespace detail
} // namespace goleta::rhi
