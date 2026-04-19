/// @file
/// @brief Cross-backend device / adapter / feature smoke tests.

#include "Common/RhiTestFixture.h"

using namespace goleta::rhi::tests;

GPU_TEST(Device, AdapterInfoPopulated, BackendMask::All)
{
    const auto& A = F.Device->adapter();
    EXPECT_STRNE(A.Name, "");
    EXPECT_EQ(A.Backend, F.GetParam());
}

GPU_TEST(Device, BackendMatchesParam, BackendMask::All)
{
    EXPECT_EQ(F.Device->backend(), F.GetParam());
}

GPU_TEST(Device, WaitIdleOnFreshDevice, BackendMask::All)
{
    EXPECT_TRUE(F.Device->waitIdle().isOk());
}

GPU_TEST(Device, TimelineSemaphoreAlwaysSupported, BackendMask::All)
{
    EXPECT_TRUE(F.Device->features().Core.TimelineSemaphore);
}

GPU_TEST(Device, EnhancedBarriersRequiredForGpuBackends, GpuOnly)
{
    EXPECT_TRUE(F.Device->features().Core.EnhancedBarriers);
    EXPECT_TRUE(F.Device->features().Core.DynamicRendering);
    EXPECT_TRUE(F.Device->features().Core.ShaderModel66DynamicResources);
}
