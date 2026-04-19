/// @file
/// @brief Unit tests for multi-queue scheduling: preferredQueue() grouping and cross-queue
///        fence wait computation.

#include <gtest/gtest.h>

#include "ExecuteSchedule.h"
#include "RHIEnums.h"
#include "RenderGraph.h"
#include "RenderPass.h"
#include "RenderPassReflection.h"

using namespace goleta;
using namespace goleta::rg;

namespace
{

class GraphicsWritePass : public RenderPass
{
public:
    PassOutput<Texture> Out;
    void reflect(RenderPassReflection& R) override
    {
        Out = R.addOutput("out")
                  .format(rhi::RhiFormat::Rgba8Unorm)
                  .extent2D(32, 32)
                  .usage(rhi::RhiTextureUsage::ColorAttachment | rhi::RhiTextureUsage::Sampled);
    }
    void execute(RenderContext&, const RenderData&) override {}
};

class ComputeReadWritePass : public RenderPass
{
public:
    PassInput<Texture>  In;
    PassOutput<Texture> Out;
    void reflect(RenderPassReflection& R) override
    {
        In  = R.addInput("in").bindAs(RgBindAs::Sampled);
        Out = R.addOutput("out")
                  .format(rhi::RhiFormat::Rgba8Unorm)
                  .extent2D(32, 32)
                  .usage(rhi::RhiTextureUsage::Storage | rhi::RhiTextureUsage::Sampled);
    }
    void execute(RenderContext&, const RenderData&) override {}
    rhi::RhiQueueKind preferredQueue() const noexcept override
    {
        return rhi::RhiQueueKind::Compute;
    }
};

class GraphicsReadPass : public RenderPass
{
public:
    PassInput<Texture>  In;
    PassOutput<Texture> Out;
    void reflect(RenderPassReflection& R) override
    {
        In  = R.addInput("in").bindAs(RgBindAs::Sampled);
        Out = R.addOutput("out")
                  .format(rhi::RhiFormat::Rgba8Unorm)
                  .extent2D(32, 32)
                  .usage(rhi::RhiTextureUsage::ColorAttachment);
    }
    void execute(RenderContext&, const RenderData&) override {}
};

const ExecuteSchedule& schedule(const RenderGraph& Rg)
{
    const ExecuteSchedule* S = Rg.debugSchedule();
    return *S;
}

} // namespace

TEST(QueueSchedulerTests, AllGraphicsProducesSingleGroup)
{
    RenderGraph Rg;
    auto* A = Rg.addPass<GraphicsWritePass>("A");
    auto* B = Rg.addPass<GraphicsReadPass>("B");
    Rg.connect(A->Out, B->In);
    Rg.markOutput(B->Out);
    ASSERT_TRUE(Rg.compile(nullptr).isOk());

    const auto& S = schedule(Rg);
    ASSERT_EQ(S.Groups.size(), 1u);
    EXPECT_EQ(S.Groups[0].QueueKind, rhi::RhiQueueKind::Graphics);
    EXPECT_EQ(S.Groups[0].Passes.size(), 2u);
    EXPECT_TRUE(S.Groups[0].WaitGroups.empty());
    EXPECT_EQ(S.FenceValuesPerFrame, 1u);
}

TEST(QueueSchedulerTests, ComputePassSplitsGroups)
{
    RenderGraph Rg;
    auto* A = Rg.addPass<GraphicsWritePass>("A");
    auto* B = Rg.addPass<ComputeReadWritePass>("B");
    auto* C = Rg.addPass<GraphicsReadPass>("C");
    Rg.connect(A->Out, B->In);
    Rg.connect(B->Out, C->In);
    Rg.markOutput(C->Out);
    ASSERT_TRUE(Rg.compile(nullptr).isOk());

    const auto& S = schedule(Rg);
    ASSERT_EQ(S.Groups.size(), 3u);
    EXPECT_EQ(S.Groups[0].QueueKind, rhi::RhiQueueKind::Graphics);
    EXPECT_EQ(S.Groups[1].QueueKind, rhi::RhiQueueKind::Compute);
    EXPECT_EQ(S.Groups[2].QueueKind, rhi::RhiQueueKind::Graphics);

    EXPECT_EQ(S.Groups[0].SignalOffset, 1u);
    EXPECT_EQ(S.Groups[1].SignalOffset, 2u);
    EXPECT_EQ(S.Groups[2].SignalOffset, 3u);

    // B (compute) depends on A (graphics) — different queues, needs wait.
    ASSERT_EQ(S.Groups[1].WaitGroups.size(), 1u);
    EXPECT_EQ(S.Groups[1].WaitGroups[0], 0u);
    // C (graphics) depends on B (compute) — different queues, needs wait.
    ASSERT_EQ(S.Groups[2].WaitGroups.size(), 1u);
    EXPECT_EQ(S.Groups[2].WaitGroups[0], 1u);
}

TEST(QueueSchedulerTests, SameQueueDiamondDoesNotEmitCrossQueueWaits)
{
    RenderGraph Rg;
    auto* A = Rg.addPass<GraphicsWritePass>("A");
    auto* B = Rg.addPass<GraphicsReadPass>("B");
    auto* C = Rg.addPass<GraphicsReadPass>("C");
    Rg.connect(A->Out, B->In);
    Rg.connect(A->Out, C->In);
    Rg.markOutput(C->Out);
    ASSERT_TRUE(Rg.compile(nullptr).isOk());

    const auto& S = schedule(Rg);
    // All on graphics queue => single group, no cross-queue waits.
    ASSERT_EQ(S.Groups.size(), 1u);
    EXPECT_TRUE(S.Groups[0].WaitGroups.empty());
}

TEST(QueueSchedulerTests, FenceOffsetsAreContiguous)
{
    RenderGraph Rg;
    auto* A = Rg.addPass<GraphicsWritePass>("A");
    auto* B = Rg.addPass<ComputeReadWritePass>("B");
    Rg.connect(A->Out, B->In);
    Rg.markOutput(B->Out);
    ASSERT_TRUE(Rg.compile(nullptr).isOk());

    const auto& S = schedule(Rg);
    ASSERT_EQ(S.Groups.size(), 2u);
    // Offsets start at 1 and are contiguous; FenceValuesPerFrame == Groups.size().
    EXPECT_EQ(S.Groups[0].SignalOffset, 1u);
    EXPECT_EQ(S.Groups[1].SignalOffset, 2u);
    EXPECT_EQ(S.FenceValuesPerFrame, 2u);
}
