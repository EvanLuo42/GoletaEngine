/// @file
/// @brief Unit tests for the barrier planner: verify per-pass transitions via the compiled
///        graph's PrePassBarriers table. Uses Internal/CompiledGraph.h through the
///        module's Internal include path.

#include <gtest/gtest.h>

#include "CompiledGraph.h"
#include "RenderGraph.h"
#include "RenderPass.h"
#include "RenderPassReflection.h"

using namespace goleta;
using namespace goleta::rg;

namespace
{

class ClearColorPass : public RenderPass
{
public:
    PassOutput<Texture> Color;
    void                reflect(RenderPassReflection& R) override
    {
        Color = R.addOutput("color")
                    .format(rhi::RhiFormat::Rgba8Unorm)
                    .extent2D(32, 32)
                    .usage(rhi::RhiTextureUsage::ColorAttachment | rhi::RhiTextureUsage::Sampled);
    }
    void execute(RenderContext&, const RenderData&) override {}
};

class SamplePass : public RenderPass
{
public:
    PassInput<Texture>  In;
    PassOutput<Texture> Out;
    void                reflect(RenderPassReflection& R) override
    {
        In  = R.addInput("in").bindAs(RgBindAs::Sampled);
        Out = R.addOutput("out")
                  .format(rhi::RhiFormat::Rgba8Unorm)
                  .extent2D(32, 32)
                  .usage(rhi::RhiTextureUsage::ColorAttachment);
    }
    void execute(RenderContext&, const RenderData&) override {}
};

const CompiledGraph& compiled(const RenderGraph& Rg)
{
    const CompiledGraph* Cg = Rg.debugCompiledGraph();
    return *Cg;
}

} // namespace

TEST(BarrierPlannerTests, FirstUseWriteBecomesDiscardingTransition)
{
    RenderGraph Rg;
    auto*       P = Rg.addPass<ClearColorPass>("clear");
    Rg.markOutput(P->Color);
    ASSERT_TRUE(Rg.compile(nullptr).isOk());

    const auto& Cg = compiled(Rg);
    ASSERT_EQ(Cg.PrePassBarriers.size(), 1u);
    const auto& Pre = Cg.PrePassBarriers[0];
    ASSERT_EQ(Pre.Textures.size(), 1u);
    EXPECT_EQ(Pre.Textures[0].SrcLayout, rhi::RhiTextureLayout::Undefined);
    EXPECT_EQ(Pre.Textures[0].DstLayout, rhi::RhiTextureLayout::ColorAttachment);
    EXPECT_TRUE(Pre.Textures[0].DiscardContents);
}

TEST(BarrierPlannerTests, WriteThenSampleTransitionsToShaderResource)
{
    RenderGraph Rg;
    auto*       C = Rg.addPass<ClearColorPass>("clear");
    auto*       S = Rg.addPass<SamplePass>("sample");
    Rg.connect(C->Color, S->In);
    Rg.markOutput(S->Out);
    ASSERT_TRUE(Rg.compile(nullptr).isOk());

    const auto& Cg = compiled(Rg);
    ASSERT_GE(Cg.PrePassBarriers.size(), 2u);

    // Pass 0 (clear): ColorAttachment first-use transition for the shared resource.
    ASSERT_EQ(Cg.PrePassBarriers[0].Textures.size(), 1u);
    EXPECT_EQ(Cg.PrePassBarriers[0].Textures[0].DstLayout, rhi::RhiTextureLayout::ColorAttachment);

    // Pass 1 (sample): the color texture transitions to ShaderResource; the sample pass's own
    // output transitions to ColorAttachment (first-use write).
    const auto& Pre1 = Cg.PrePassBarriers[1];
    ASSERT_GE(Pre1.Textures.size(), 2u);
    bool FoundShaderResource = false;
    bool FoundNewColor       = false;
    for (const auto& T : Pre1.Textures)
    {
        if (T.DstLayout == rhi::RhiTextureLayout::ShaderResource &&
            T.SrcLayout == rhi::RhiTextureLayout::ColorAttachment)
            FoundShaderResource = true;
        if (T.DstLayout == rhi::RhiTextureLayout::ColorAttachment && T.SrcLayout == rhi::RhiTextureLayout::Undefined &&
            T.DiscardContents)
            FoundNewColor = true;
    }
    EXPECT_TRUE(FoundShaderResource);
    EXPECT_TRUE(FoundNewColor);
}

TEST(BarrierPlannerTests, MarkOutputEmitsFinalTransition)
{
    RenderGraph Rg;
    auto*       P = Rg.addPass<ClearColorPass>("clear");
    Rg.markOutput(P->Color, rhi::RhiTextureLayout::Present);
    ASSERT_TRUE(Rg.compile(nullptr).isOk());

    const auto& Cg = compiled(Rg);
    ASSERT_EQ(Cg.FinalBarriers.Textures.size(), 1u);
    EXPECT_EQ(Cg.FinalBarriers.Textures[0].DstLayout, rhi::RhiTextureLayout::Present);
    EXPECT_EQ(Cg.FinalBarriers.Textures[0].DstAccess, rhi::RhiAccess::Present);
}
