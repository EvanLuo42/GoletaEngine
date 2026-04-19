#include <gtest/gtest.h>

#include "RHIBuffer.h"
#include "RHICommandList.h"
#include "RHIDebug.h"
#include "RHIDevice.h"
#include "RHIInstance.h"
#include "RHIQueue.h"
#include "RHIRayTracing.h"
#include "RHISampler.h"
#include "RHISwapChain.h"
#include "RHISync.h"
#include "RHITexture.h"

using namespace goleta;
using namespace goleta::rhi;

namespace
{

Rc<IRhiDevice> createNullDeviceForTest(bool WithRayTracing = false)
{
    RhiInstanceCreateInfo II{};
    II.Backend    = BackendKind::Null;
    II.DebugLevel = RhiDebugLevel::Validation;
    auto Instance = createInstance(II);
    EXPECT_FALSE(Instance.isNull());
    if (Instance.isNull())
        return Rc<IRhiDevice>{};

    RhiDeviceCreateInfo DI{};
    DI.RequestRayTracing = WithRayTracing;
    return Instance->createDevice(DI);
}

} // namespace

TEST(RHINullBackendTests, EndToEndFrame)
{
    auto Device = createNullDeviceForTest();
    ASSERT_FALSE(Device.isNull());
    EXPECT_EQ(Device->backend(), BackendKind::Null);
    EXPECT_TRUE(Device->features().Core.TimelineSemaphore);
    EXPECT_TRUE(Device->features().Core.EnhancedBarriers);
    EXPECT_TRUE(Device->features().BindlessResources.Supported);

    auto GfxQ = Device->getQueue(RhiQueueKind::Graphics);
    ASSERT_FALSE(GfxQ.isNull());
    EXPECT_EQ(GfxQ->queueKind(), RhiQueueKind::Graphics);

    auto Fence = Device->createFence(0);
    ASSERT_FALSE(Fence.isNull());

    RhiBufferDesc BD{};
    BD.SizeBytes = 1024;
    BD.Usage     = RhiBufferUsage::VertexBuffer | RhiBufferUsage::CopyDest;
    BD.Location  = RhiMemoryLocation::Upload;
    auto Buf     = Device->createBuffer(BD);
    ASSERT_FALSE(Buf.isNull());
    EXPECT_EQ(Buf->desc().SizeBytes, 1024u);

    void* Mapped = Buf->map(0, 64);
    ASSERT_NE(Mapped, nullptr);
    Buf->unmap();

    RhiTextureDesc TD{};
    TD.Width  = 64;
    TD.Height = 64;
    TD.Format = RhiFormat::Rgba8Unorm;
    TD.Usage  = RhiTextureUsage::Sampled | RhiTextureUsage::ColorAttachment;
    auto Tex  = Device->createTexture(TD);
    ASSERT_FALSE(Tex.isNull());

    auto Pool = Device->createCommandPool(RhiQueueKind::Graphics);
    ASSERT_FALSE(Pool.isNull());
    EXPECT_EQ(Pool->queueKind(), RhiQueueKind::Graphics);
    auto List = Pool->allocate();
    ASSERT_FALSE(List.isNull());
    List->begin();
    List->beginDebugScope("GBuffer", 0xff00ff00);
    List->endDebugScope();
    List->end();

    IRhiCommandList* Raw = List.get();
    RhiFenceSignal   Signal{Fence.get(), 1};
    RhiSubmitInfo    Info{};
    Info.CommandLists     = &Raw;
    Info.CommandListCount = 1;
    Info.SignalFences     = &Signal;
    Info.SignalFenceCount = 1;
    EXPECT_TRUE(GfxQ->submit(Info).isOk());

    const auto WaitR = Fence->wait(1);
    ASSERT_TRUE(WaitR.isOk());
    EXPECT_EQ(WaitR.value(), RhiWaitStatus::Reached);
    EXPECT_GE(Fence->completedValue(), 1u);
}

TEST(RHINullBackendTests, RayTracingGatedOffByDefault)
{
    auto Device = createNullDeviceForTest(/*WithRayTracing=*/false);
    ASSERT_FALSE(Device.isNull());
    EXPECT_FALSE(Device->features().RayTracing.Supported);

    RhiAccelStructureDesc AD{};
    AD.Kind    = RhiAccelStructureKind::BottomLevel;
    auto Accel = Device->createAccelStructure(AD);
    EXPECT_TRUE(Accel.isNull());
}

TEST(RHINullBackendTests, RayTracingEnabledWhenRequested)
{
    auto Device = createNullDeviceForTest(/*WithRayTracing=*/true);
    ASSERT_FALSE(Device.isNull());
    EXPECT_TRUE(Device->features().RayTracing.Supported);

    RhiAccelStructureDesc AD{};
    AD.Kind    = RhiAccelStructureKind::BottomLevel;
    auto Accel = Device->createAccelStructure(AD);
    EXPECT_FALSE(Accel.isNull());
}

TEST(RHINullBackendTests, SwapChainCyclesImages)
{
    auto             Device = createNullDeviceForTest();
    RhiSwapChainDesc SD{};
    SD.Width        = 800;
    SD.Height       = 600;
    SD.ImageCount   = 3;
    SD.Format       = RhiFormat::Bgra8Unorm;
    SD.NativeWindow.Kind   = RhiNativeWindowKind::Win32Hwnd;
    SD.NativeWindow.Handle = reinterpret_cast<void*>(uintptr_t{0xDEADBEEF});

    auto Swap = Device->createSwapChain(SD);
    ASSERT_FALSE(Swap.isNull());
    EXPECT_EQ(Swap->imageCount(), 3u);

    for (uint32_t I = 0; I < 3; ++I)
    {
        ASSERT_NE(Swap->image(I), nullptr);
    }

    const auto First  = Swap->acquireNextImage();
    const auto Second = Swap->acquireNextImage();
    ASSERT_TRUE(First.isOk());
    ASSERT_TRUE(Second.isOk());
    EXPECT_NE(First.value(), Second.value());
}

TEST(RHINullBackendTests, ObjectNaming)
{
    auto          Device = createNullDeviceForTest();
    RhiBufferDesc BD{};
    BD.SizeBytes = 16;
    BD.DebugName = "TestBuffer";
    auto Buf     = Device->createBuffer(BD);
    EXPECT_STREQ(Buf->debugName(), "TestBuffer");
    Buf->setDebugName("RenamedBuffer");
    EXPECT_STREQ(Buf->debugName(), "RenamedBuffer");
    Buf->setDebugName(nullptr);
    EXPECT_STREQ(Buf->debugName(), "");
}
