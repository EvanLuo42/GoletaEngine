/// @file
/// @brief Queue access and empty-submit semantics across backends.

#include "Common/RhiTestFixture.h"

using namespace goleta::rhi::tests;

GPU_TEST(Queue, GetReturnsCorrectKind, BackendMask::All)
{
    EXPECT_EQ(F.Device->getQueue(RhiQueueKind::Graphics)->queueKind(), RhiQueueKind::Graphics);
    EXPECT_EQ(F.Device->getQueue(RhiQueueKind::Compute)->queueKind(),  RhiQueueKind::Compute);
    EXPECT_EQ(F.Device->getQueue(RhiQueueKind::Copy)->queueKind(),     RhiQueueKind::Copy);
}

GPU_TEST(Queue, EmptySubmitReachesFence, BackendMask::All)
{
    auto Fence = F.Device->createFence(0);
    auto Pool  = F.Device->createCommandPool(RhiQueueKind::Graphics);
    auto Cl    = Pool->allocate();
    ASSERT_TRUE(Fence); ASSERT_TRUE(Cl);
    Cl->begin();
    Cl->end();

    IRhiCommandList* Lists[] = {Cl.get()};
    RhiFenceSignal Sig{Fence.get(), 1};
    RhiSubmitInfo  Info{};
    Info.CommandLists     = Lists;
    Info.CommandListCount = 1;
    Info.SignalFences     = &Sig;
    Info.SignalFenceCount = 1;

    EXPECT_TRUE(F.Gfx->submit(Info).isOk());
    const auto W = Fence->wait(1);
    ASSERT_TRUE(W.isOk());
    EXPECT_EQ(W.value(), RhiWaitStatus::Reached);
}

GPU_TEST(Queue, WaitIdleOnEmptyCompletes, BackendMask::All)
{
    EXPECT_TRUE(F.Gfx->waitIdle().isOk());
}
