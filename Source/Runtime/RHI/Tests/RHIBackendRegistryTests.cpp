#include <gtest/gtest.h>

#include "RHIBackend.h"
#include "RHIBackendRegistry.h"
#include "RHIInstance.h"

using namespace goleta;
using namespace goleta::rhi;

namespace
{

Rc<IRhiInstance> fakeFactory(const RhiInstanceCreateInfo&)
{
    return Rc<IRhiInstance>{};   // Returns null; we just care registration was observed.
}

} // namespace

TEST(RHIBackendRegistryTests, NullBackendIsAlwaysRegistered)
{
    // The module's Private/RHINullBackend.cpp registers at static-init. Never reset the
    // registry in this test; just confirm the null factory can be reached.
    EXPECT_NE(findRhiBackend(BackendKind::Null), nullptr);
}

TEST(RHIBackendRegistryTests, UnregisteredBackendReturnsNullptr)
{
    // D3D12 / Vulkan / console backends are not linked into Engine.Tests.
    EXPECT_EQ(findRhiBackend(BackendKind::D3D12),     nullptr);
    EXPECT_EQ(findRhiBackend(BackendKind::Vulkan),    nullptr);
    EXPECT_EQ(findRhiBackend(BackendKind::D3D12Xbox), nullptr);
    EXPECT_EQ(findRhiBackend(BackendKind::GNMX),      nullptr);
    EXPECT_EQ(findRhiBackend(BackendKind::NVN),       nullptr);
}

TEST(RHIBackendRegistryTests, CreateInstanceWithUnregisteredBackendReturnsNull)
{
    RhiInstanceCreateInfo Info{};
    Info.Backend = BackendKind::D3D12;
    Rc<IRhiInstance> Instance = createInstance(Info);
    EXPECT_TRUE(Instance.isNull());
}

TEST(RHIBackendRegistryTests, CreateInstanceWithNullBackendSucceeds)
{
    RhiInstanceCreateInfo Info{};
    Info.Backend = BackendKind::Null;
    Rc<IRhiInstance> Instance = createInstance(Info);
    ASSERT_FALSE(Instance.isNull());
    EXPECT_EQ(Instance->backend(), BackendKind::Null);

    const auto Adapters = Instance->enumerateAdapters();
    EXPECT_EQ(Adapters.size(), 1u);
    EXPECT_EQ(Adapters.front().Backend, BackendKind::Null);
}

TEST(RHIBackendRegistryTests, DuplicateRegistrationIsIgnored)
{
    const uint32_t Before = detail::rhiBackendRegistrationCount();
    RhiBackendEntry Dup{};
    Dup.Kind    = BackendKind::Null;
    Dup.Factory = &fakeFactory;
    Dup.Name    = "DuplicateNull";
    registerRhiBackend(Dup);
    const uint32_t After = detail::rhiBackendRegistrationCount();
    EXPECT_EQ(Before, After);
    // The original Null factory (not fakeFactory) must still produce a real instance.
    RhiInstanceCreateInfo Info{};
    Info.Backend = BackendKind::Null;
    Rc<IRhiInstance> Instance = createInstance(Info);
    EXPECT_FALSE(Instance.isNull());
}
