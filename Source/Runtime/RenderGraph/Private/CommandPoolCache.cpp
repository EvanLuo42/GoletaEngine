#include "CommandPoolCache.h"

#include "RHIDevice.h"

namespace goleta::rg
{

rhi::IRhiCommandPool* CommandPoolCache::acquire(rhi::IRhiDevice&  Device,
                                                rhi::RhiQueueKind Queue)
{
    const Key K{std::this_thread::get_id(), Queue};
    std::lock_guard<std::mutex> Lock(Mutex_);
    auto It = Pools_.find(K);
    if (It != Pools_.end())
        return It->second.get();
    Rc<rhi::IRhiCommandPool> New = Device.createCommandPool(Queue);
    rhi::IRhiCommandPool*    Raw = New.get();
    Pools_.emplace(K, std::move(New));
    return Raw;
}

void CommandPoolCache::resetAll()
{
    std::lock_guard<std::mutex> Lock(Mutex_);
    for (auto& [K, Pool] : Pools_)
    {
        if (Pool)
            Pool->reset();
    }
}

void CommandPoolCache::clear() noexcept
{
    std::lock_guard<std::mutex> Lock(Mutex_);
    Pools_.clear();
}

} // namespace goleta::rg
