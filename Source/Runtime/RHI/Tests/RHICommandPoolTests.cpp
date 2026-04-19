#include <gtest/gtest.h>

#include <atomic>
#include <thread>
#include <vector>

#include "RHICommandList.h"
#include "RHIDevice.h"
#include "RHIInstance.h"
#include "RHIQueue.h"
#include "RHISync.h"

using namespace goleta;
using namespace goleta::rhi;

namespace
{

Rc<IRhiDevice> makeDevice()
{
    RhiInstanceCreateInfo II{};
    II.Backend    = BackendKind::Null;
    auto Instance = createInstance(II);
    EXPECT_FALSE(Instance.isNull());
    RhiDeviceCreateInfo DI{};
    return Instance->createDevice(DI);
}

} // namespace

TEST(RHICommandPoolTests, PoolTracksQueueKind)
{
    auto Device = makeDevice();
    for (auto Kind : {RhiQueueKind::Graphics, RhiQueueKind::Compute, RhiQueueKind::Copy})
    {
        auto Pool = Device->createCommandPool(Kind);
        ASSERT_FALSE(Pool.isNull());
        EXPECT_EQ(Pool->queueKind(), Kind);
    }
}

TEST(RHICommandPoolTests, AllocatedListInheritsQueueKind)
{
    auto Device = makeDevice();
    auto Pool   = Device->createCommandPool(RhiQueueKind::Compute);
    auto List   = Pool->allocate();
    ASSERT_FALSE(List.isNull());
    EXPECT_EQ(List->queueKind(), RhiQueueKind::Compute);
}

TEST(RHICommandPoolTests, AllocateMultipleAndResetDoesNotCrash)
{
    auto Device = makeDevice();
    auto Pool   = Device->createCommandPool(RhiQueueKind::Graphics);

    std::vector<Rc<IRhiCommandList>> Lists;
    for (int I = 0; I < 16; ++I)
        Lists.emplace_back(Pool->allocate());
    for (auto& L : Lists)
    {
        L->begin();
        L->end();
    }
    Lists.clear();
    Pool->reset();

    auto After = Pool->allocate();
    ASSERT_FALSE(After.isNull());
}

TEST(RHICommandPoolTests, MultiThreadedRecordingUsesSeparatePoolsPerThread)
{
    auto Device = makeDevice();
    auto Queue  = Device->getQueue(RhiQueueKind::Graphics);
    auto Fence  = Device->createFence(0);

    constexpr int ThreadCount    = 4;
    constexpr int ListsPerThread = 8;

    std::vector<std::vector<Rc<IRhiCommandList>>> PerThreadLists(ThreadCount);
    std::vector<std::thread>                      Workers;
    std::atomic<int>                              RecordedLists{0};

    for (int T = 0; T < ThreadCount; ++T)
    {
        Workers.emplace_back(
            [&, T]()
            {
                Rc<IRhiCommandPool> Pool = Device->createCommandPool(RhiQueueKind::Graphics);
                for (int I = 0; I < ListsPerThread; ++I)
                {
                    Rc<IRhiCommandList> List = Pool->allocate();
                    List->begin();
                    List->beginDebugScope("Worker", 0);
                    List->draw(3, 1, 0, 0);
                    List->endDebugScope();
                    List->end();
                    PerThreadLists[T].emplace_back(std::move(List));
                    RecordedLists.fetch_add(1);
                }
            });
    }
    for (auto& W : Workers)
        W.join();

    EXPECT_EQ(RecordedLists.load(), ThreadCount * ListsPerThread);

    std::vector<IRhiCommandList*> RawLists;
    for (auto& Bucket : PerThreadLists)
        for (auto& L : Bucket)
            RawLists.push_back(L.get());

    RhiFenceSignal Signal{Fence.get(), 1};
    RhiSubmitInfo  Info{};
    Info.CommandLists     = RawLists.data();
    Info.CommandListCount = static_cast<uint32_t>(RawLists.size());
    Info.SignalFences     = &Signal;
    Info.SignalFenceCount = 1;
    EXPECT_TRUE(Queue->submit(Info).isOk());

    const auto WaitR = Fence->wait(1);
    ASSERT_TRUE(WaitR.isOk());
    EXPECT_EQ(WaitR.value(), RhiWaitStatus::Reached);
}
