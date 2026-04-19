/// @file
/// @brief Ray-tracing gating. Each backend reports its own RT support; tests that require the
///        feature flip on via RequestRayTracing live in the Null-only lane for MVP since the
///        D3D12 backend's DXR path is still scaffolded.

#include <gtest/gtest.h>

#include "Common/RhiTestFixture.h"
#include "RHIRayTracing.h"

using namespace goleta::rhi::tests;

GPU_TEST(RayTracing, GatedOffByDefault, BackendMask::All)
{
    // MVP: every backend reports RT unsupported when RequestRayTracing isn't set.
    EXPECT_FALSE(F.Device->features().RayTracing.Supported);

    RhiAccelStructureDesc AD{};
    AD.Kind = RhiAccelStructureKind::BottomLevel;
    auto Accel = F.Device->createAccelStructure(AD);
    EXPECT_TRUE(Accel.isNull());
}

// RayTracing toggles on via RequestRayTracing only on backends whose device builder honours
// the flag. MVP D3D12 doesn't; Null does. Runs as a standalone TEST so it can spin up its own
// instance + device with the feature requested.
TEST(RayTracing, EnabledWhenRequestedOnNull)
{
    RhiInstanceCreateInfo II{};
    II.Backend = BackendKind::Null;
    auto Instance = createInstance(II);
    ASSERT_TRUE(Instance);

    RhiDeviceCreateInfo DI{};
    DI.RequestRayTracing = true;
    auto Device = Instance->createDevice(DI);
    ASSERT_TRUE(Device);
    EXPECT_TRUE(Device->features().RayTracing.Supported);

    RhiAccelStructureDesc AD{};
    AD.Kind = RhiAccelStructureKind::BottomLevel;
    auto Accel = Device->createAccelStructure(AD);
    EXPECT_FALSE(Accel.isNull());
}
