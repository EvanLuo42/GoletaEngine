/// @file
/// @brief Timeline-fence semantics across backends. These run on the Null backend too since it
///        implements IRhiFence via atomic<uint64>.

#include "Common/RhiTestFixture.h"

using namespace goleta::rhi::tests;

GPU_TEST(Fence, InitialValueReflected, BackendMask::All)
{
    auto Fence = F.Device->createFence(42);
    ASSERT_TRUE(Fence);
    EXPECT_EQ(Fence->completedValue(), 42u);
}

GPU_TEST(Fence, SignalFromCpuAdvances, BackendMask::All)
{
    auto Fence = F.Device->createFence(0);
    ASSERT_TRUE(Fence);
    EXPECT_TRUE(Fence->signalFromCpu(7).isOk());
    EXPECT_EQ(Fence->completedValue(), 7u);
}

GPU_TEST(Fence, WaitReturnsReachedAfterSignal, BackendMask::All)
{
    auto Fence = F.Device->createFence(0);
    ASSERT_TRUE(Fence);
    EXPECT_TRUE(Fence->signalFromCpu(3).isOk());
    const auto R = Fence->wait(3);
    ASSERT_TRUE(R.isOk());
    EXPECT_EQ(R.value(), RhiWaitStatus::Reached);
}

GPU_TEST(Fence, WaitTimesOutForFutureValue, GpuOnly)
{
    // Skipped for Null since NullFence doesn't block; it always returns immediately.
    auto Fence = F.Device->createFence(0);
    ASSERT_TRUE(Fence);
    const auto R = Fence->wait(99, /*TimeoutNanos=*/1'000'000ull);
    ASSERT_TRUE(R.isOk());
    EXPECT_EQ(R.value(), RhiWaitStatus::TimedOut);
}
