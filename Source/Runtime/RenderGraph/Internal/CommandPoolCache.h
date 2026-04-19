#pragma once

/// @file
/// @brief Per-(thread, queue-kind) command pool cache. Lazily creates pools on first use,
///        tracks which pools were touched in the current frame so they can be reset as a
///        batch after their work finishes on the GPU.

#include <cstdint>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <vector>

#include "Memory/Rc.h"
#include "RHICommandList.h"
#include "RHIEnums.h"

namespace goleta::rhi
{
class IRhiDevice;
}

namespace goleta::rg
{

class CommandPoolCache
{
public:
    /// @brief Key combining calling thread and queue kind.
    struct Key
    {
        std::thread::id   Thread;
        rhi::RhiQueueKind Queue;

        bool operator==(const Key& O) const noexcept
        {
            return Thread == O.Thread && Queue == O.Queue;
        }
    };

    struct KeyHash
    {
        size_t operator()(const Key& K) const noexcept
        {
            return std::hash<std::thread::id>{}(K.Thread) ^
                   (static_cast<size_t>(K.Queue) << 16);
        }
    };

    /// @brief Fetch or create a pool for (current thread, @p Queue). Thread-safe. Returns the
    ///        same pool on repeated calls from the same thread.
    rhi::IRhiCommandPool* acquire(rhi::IRhiDevice& Device, rhi::RhiQueueKind Queue);

    /// @brief Reset every pool whose GPU work has completed. Callers are responsible for
    ///        ensuring the supplied fence value has been reached before calling. Safe to call
    ///        from any thread; the cache takes the internal mutex during reset.
    void resetAll();

    /// @brief Drop all cached pools. Call before device shutdown.
    void clear() noexcept;

private:
    std::mutex                                                 Mutex_;
    std::unordered_map<Key, Rc<rhi::IRhiCommandPool>, KeyHash> Pools_;
};

} // namespace goleta::rg
