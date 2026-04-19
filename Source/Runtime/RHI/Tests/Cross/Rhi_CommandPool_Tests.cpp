/// @file
/// @brief Command-pool lifecycle + thread-local recording. Ported from RHICommandPoolTests.cpp
///        into the cross-backend framework so it runs on every registered backend.

#include <atomic>
#include <thread>
#include <vector>

#include "Common/RhiTestFixture.h"
#include "Common/RhiTestHelpers.h"

using namespace goleta::rhi::tests;

GPU_TEST(CommandPool, PoolTracksQueueKind, BackendMask::All)
{
    for (auto Kind : {RhiQueueKind::Graphics, RhiQueueKind::Compute, RhiQueueKind::Copy})
    {
        auto Pool = F.Device->createCommandPool(Kind);
        ASSERT_TRUE(Pool);
        EXPECT_EQ(Pool->queueKind(), Kind);
    }
}

GPU_TEST(CommandPool, AllocatedListInheritsQueueKind, BackendMask::All)
{
    auto Pool = F.Device->createCommandPool(RhiQueueKind::Compute);
    auto List = Pool->allocate();
    ASSERT_TRUE(List);
    EXPECT_EQ(List->queueKind(), RhiQueueKind::Compute);
}

GPU_TEST(CommandPool, AllocateMultipleAndResetDoesNotCrash, BackendMask::All)
{
    auto Pool = F.Device->createCommandPool(RhiQueueKind::Graphics);

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
    ASSERT_TRUE(After);
}

GPU_TEST(CommandPool, MultiThreadedRecordingUsesSeparatePoolsPerThread, BackendMask::All)
{
    auto Fence = F.Device->createFence(0);
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
                auto Pool = F.Device->createCommandPool(RhiQueueKind::Graphics);
                for (int I = 0; I < ListsPerThread; ++I)
                {
                    auto List = Pool->allocate();
                    List->begin();
                    List->beginDebugScope("Worker", 0);
                    // No draw here — cross-backend tests can't issue a valid draw without a
                    // bound pipeline + render pass, and the D3D12 debug layer would reject it.
                    List->endDebugScope();
                    List->end();
                    PerThreadLists[T].emplace_back(std::move(List));
                    RecordedLists.fetch_add(1);
                }
            });
    }
    for (auto& W : Workers) W.join();

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
    EXPECT_TRUE(F.Gfx->submit(Info).isOk());

    const auto WaitR = Fence->wait(1);
    ASSERT_TRUE(WaitR.isOk());
    EXPECT_EQ(WaitR.value(), RhiWaitStatus::Reached);
}
