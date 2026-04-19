#include <gtest/gtest.h>

#include <atomic>

#include "RHIDebug.h"
#include "RHIDevice.h"
#include "RHIInstance.h"

using namespace goleta;
using namespace goleta::rhi;

namespace
{

Rc<IRhiDevice> makeNullDevice(RhiDebugLevel Level, RhiDebugCallback Callback, void* User)
{
    RhiInstanceCreateInfo II{};
    II.Backend             = BackendKind::Null;
    II.DebugLevel          = Level;
    II.MessageCallback     = Callback;
    II.MessageCallbackUser = User;
    Rc<IRhiInstance> Inst  = createInstance(II);
    EXPECT_FALSE(Inst.isNull());
    if (Inst.isNull())
        return Rc<IRhiDevice>{};

    RhiDeviceCreateInfo DI{};
    return Inst->createDevice(DI);
}

} // namespace

TEST(RHIDebugTests, DebugIsReachableAtNoneLevel)
{
    Rc<IRhiDevice> Device = makeNullDevice(RhiDebugLevel::None, nullptr, nullptr);
    ASSERT_FALSE(Device.isNull());
    Rc<IRhiDebug> Debug = Device->debug();
    ASSERT_FALSE(Debug.isNull());
    EXPECT_EQ(Debug->level(), RhiDebugLevel::None);
}

TEST(RHIDebugTests, FilterStackNestsAndPops)
{
    Rc<IRhiDevice> Device = makeNullDevice(RhiDebugLevel::Validation, nullptr, nullptr);
    Rc<IRhiDebug>  Debug  = Device->debug();
    ASSERT_FALSE(Debug.isNull());

    Debug->pushMessageFilter(100, true);
    Debug->pushMessageFilter(200, false);
    Debug->pushMessageFilter(300, true);
    Debug->popMessageFilter();
    Debug->popMessageFilter();
    Debug->popMessageFilter();
    // Extra pop is tolerated (no underflow).
    Debug->popMessageFilter();
    SUCCEED();
}

TEST(RHIDebugTests, BreadcrumbInsertIsHarmless)
{
    Rc<IRhiDevice> Device = makeNullDevice(RhiDebugLevel::Full, nullptr, nullptr);
    Rc<IRhiDebug>  Debug  = Device->debug();
    Debug->insertBreadcrumb("frame-start");
    Debug->insertBreadcrumb("upload-geometry");
    Debug->insertBreadcrumb(nullptr); // Must not crash.
    SUCCEED();
}

TEST(RHIDebugTests, CrashReportReportsNoneWhenHealthy)
{
    Rc<IRhiDevice> Device = makeNullDevice(RhiDebugLevel::Full, nullptr, nullptr);
    Rc<IRhiDebug>  Debug  = Device->debug();
    RhiCrashReport Report{};
    EXPECT_FALSE(Debug->tryGetLastCrashReport(Report));
}

TEST(RHIDebugTests, CaptureBeginEndAreNoops)
{
    Rc<IRhiDevice> Device = makeNullDevice(RhiDebugLevel::Basic, nullptr, nullptr);
    Rc<IRhiDebug>  Debug  = Device->debug();
    Debug->beginCapture("snap");
    Debug->endCapture();
    SUCCEED();
}
