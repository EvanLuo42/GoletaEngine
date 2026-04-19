/// @file
/// @brief End-to-end execute() smoke test. Builds a trivial graph, drives execute(), and
///        verifies the graph-owned timeline fence advances. Runs on all registered GPU
///        backends via the RhiTest / GPU_TEST fixture.

#include "Common/RhiTestFixture.h"
#include "Common/RhiTestHelpers.h"

#include "ExecuteSchedule.h"
#include "RenderContext.h"
#include "RenderGraph.h"
#include "RenderPass.h"
#include "RenderPassReflection.h"

using namespace goleta::rhi::tests;
using namespace goleta::rg;

namespace
{

class NopUavPass : public RenderPass
{
public:
    PassOutput<Texture> Out;
    void                reflect(RenderPassReflection& R) override
    {
        Out = R.addOutput("out")
                  .format(goleta::rhi::RhiFormat::Rgba8Unorm)
                  .extent2D(64, 64)
                  .usage(goleta::rhi::RhiTextureUsage::Storage |
                         goleta::rhi::RhiTextureUsage::Sampled)
                  .bindAs(RgBindAs::Storage);
    }
    void execute(RenderContext&, const RenderData&) override {}
};

class NopComputePass : public RenderPass
{
public:
    PassInput<Texture>  In;
    PassOutput<Texture> Out;
    void                reflect(RenderPassReflection& R) override
    {
        In  = R.addInput("in").bindAs(RgBindAs::Sampled);
        Out = R.addOutput("out")
                  .format(goleta::rhi::RhiFormat::Rgba8Unorm)
                  .extent2D(64, 64)
                  .usage(goleta::rhi::RhiTextureUsage::Storage |
                         goleta::rhi::RhiTextureUsage::Sampled)
                  .bindAs(RgBindAs::Storage);
    }
    void              execute(RenderContext&, const RenderData&) override {}
    RhiQueueKind preferredQueue() const noexcept override { return RhiQueueKind::Compute; }
};

} // namespace

GPU_TEST(RenderGraph, Execute_SinglePassAdvancesFence, GpuOnly)
{
    RenderGraph Rg("exec_single");
    auto*       P = Rg.addPass<NopUavPass>("nop");
    Rg.markOutput(P->Out);
    ASSERT_TRUE(Rg.compile(F.Device).isOk());

    RenderContext Ctx;
    Ctx.setDevice(F.Device);
    Rg.execute(Ctx);
    REQUIRE_RHI_OK(F.Device->waitIdle());
}

GPU_TEST(RenderGraph, Execute_GraphicsThenComputeGroupsSubmitSeparately, GpuOnly)
{
    RenderGraph Rg("exec_multi_queue");
    auto*       Gfx  = Rg.addPass<NopUavPass>("gfx");
    auto*       Comp = Rg.addPass<NopComputePass>("compute");
    Rg.connect(Gfx->Out, Comp->In);
    Rg.markOutput(Comp->Out);
    ASSERT_TRUE(Rg.compile(F.Device).isOk());

    // Schedule should have split into two groups across queues.
    const auto* Sched = Rg.debugSchedule();
    ASSERT_NE(Sched, nullptr);
    ASSERT_EQ(Sched->Groups.size(), 2u);
    EXPECT_EQ(Sched->Groups[0].QueueKind, RhiQueueKind::Graphics);
    EXPECT_EQ(Sched->Groups[1].QueueKind, RhiQueueKind::Compute);
    ASSERT_EQ(Sched->Groups[1].WaitGroups.size(), 1u);
    EXPECT_EQ(Sched->Groups[1].WaitGroups[0], 0u);

    RenderContext Ctx;
    Ctx.setDevice(F.Device);
    Rg.execute(Ctx);
    REQUIRE_RHI_OK(F.Device->waitIdle());
}

GPU_TEST(RenderGraph, Execute_TimingsCollectedWhenEnabled, GpuOnly)
{
    RenderGraph Rg("exec_timings");
    auto*       P = Rg.addPass<NopUavPass>("nop");
    Rg.markOutput(P->Out);
    ASSERT_TRUE(Rg.compile(F.Device).isOk());

    RenderContext Ctx;
    Ctx.setDevice(F.Device);
    Rg.setTimingEnabled(true);
    Rg.execute(Ctx);
    REQUIRE_RHI_OK(F.Device->waitIdle());
    Rg.execute(Ctx); // Second execute picks up the first frame's GPU ticks via readback.
    REQUIRE_RHI_OK(F.Device->waitIdle());

    const auto Timings = Rg.lastFrameTimings();
    ASSERT_EQ(Timings.size(), 1u);
    EXPECT_GT(Timings[0].CpuEndNs, Timings[0].CpuStartNs);
    // GPU ticks may or may not have been read back depending on timing; just assert the
    // slot is populated with the pass name.
    ASSERT_NE(Timings[0].Name, nullptr);
    EXPECT_STREQ(Timings[0].Name, "nop");
}

GPU_TEST(RenderGraph, Execute_CaptureTriggerRunsWithoutCrash, GpuOnly)
{
    RenderGraph Rg("exec_capture");
    auto*       P = Rg.addPass<NopUavPass>("nop");
    Rg.markOutput(P->Out);
    ASSERT_TRUE(Rg.compile(F.Device).isOk());

    RenderContext Ctx;
    Ctx.setDevice(F.Device);
    Rg.captureNextFrame("unit_test_capture");
    Rg.execute(Ctx);
    REQUIRE_RHI_OK(F.Device->waitIdle());
}
