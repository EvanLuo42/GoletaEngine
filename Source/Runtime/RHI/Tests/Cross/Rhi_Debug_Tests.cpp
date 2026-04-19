/// @file
/// @brief Debug facade tests. Ported from RHIDebugTests.cpp to run cross-backend.

#include <gtest/gtest.h>

#include "Common/RhiTestFixture.h"

using namespace goleta::rhi::tests;

GPU_TEST(Debug, FacadeReachableAndLevelMatches, BackendMask::All)
{
    auto Debug = F.Device->debug();
    ASSERT_TRUE(Debug);
    // The shared test environment spins up with Validation; every registered backend should
    // honour at least that. None-level is covered separately via a standalone instance below.
    EXPECT_GE(static_cast<int>(Debug->level()), static_cast<int>(RhiDebugLevel::Validation));
}

GPU_TEST(Debug, FilterStackNestsAndPops, BackendMask::All)
{
    auto Debug = F.Device->debug();
    Debug->pushMessageFilter(100, true);
    Debug->pushMessageFilter(200, false);
    Debug->pushMessageFilter(300, true);
    Debug->popMessageFilter();
    Debug->popMessageFilter();
    Debug->popMessageFilter();
    Debug->popMessageFilter(); // Extra pop tolerated (no underflow).
    SUCCEED();
}

GPU_TEST(Debug, BreadcrumbInsertIsHarmless, BackendMask::All)
{
    auto Debug = F.Device->debug();
    Debug->insertBreadcrumb("frame-start");
    Debug->insertBreadcrumb("upload-geometry");
    Debug->insertBreadcrumb(nullptr);
    SUCCEED();
}

GPU_TEST(Debug, CrashReportReportsNoneWhenHealthy, BackendMask::All)
{
    auto Debug = F.Device->debug();
    RhiCrashReport Report{};
    EXPECT_FALSE(Debug->tryGetLastCrashReport(Report));
}

GPU_TEST(Debug, CaptureBeginEndAreNoops, BackendMask::All)
{
    auto Debug = F.Device->debug();
    Debug->beginCapture("snap");
    Debug->endCapture();
    SUCCEED();
}

// None-level debug flow isn't parameterized because it spawns its own instance outside the
// shared Validation-level fixture.
TEST(Debug, DebugIsReachableAtNoneLevelOnNull)
{
    RhiInstanceCreateInfo II{};
    II.Backend    = BackendKind::Null;
    II.DebugLevel = RhiDebugLevel::None;
    auto Instance = createInstance(II);
    ASSERT_TRUE(Instance);
    auto Device = Instance->createDevice(RhiDeviceCreateInfo{});
    ASSERT_TRUE(Device);
    auto Dbg = Device->debug();
    ASSERT_TRUE(Dbg);
    EXPECT_EQ(Dbg->level(), RhiDebugLevel::None);
}
