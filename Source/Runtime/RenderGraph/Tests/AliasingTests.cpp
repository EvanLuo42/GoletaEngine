/// @file
/// @brief Unit tests for intra-frame aliasing machinery: AcquireAtStart / ReleaseAfter lists
///        on each ScheduleGroup and debug-name propagation into the CompiledGraph.

#include <gtest/gtest.h>

#include "CompiledGraph.h"
#include "ExecuteSchedule.h"
#include "RenderGraph.h"
#include "RenderPass.h"
#include "RenderPassReflection.h"

using namespace goleta;
using namespace goleta::rg;

namespace
{

class WritePass : public RenderPass
{
public:
    PassOutput<Texture> Out;
    void                reflect(RenderPassReflection& R) override
    {
        Out = R.addOutput("out")
                  .format(rhi::RhiFormat::Rgba8Unorm)
                  .extent2D(64, 64)
                  .usage(rhi::RhiTextureUsage::ColorAttachment | rhi::RhiTextureUsage::Sampled);
    }
    void execute(RenderContext&, const RenderData&) override {}
};

class ReadPass : public RenderPass
{
public:
    PassInput<Texture>  In;
    PassOutput<Texture> Out;
    void                reflect(RenderPassReflection& R) override
    {
        In  = R.addInput("in").bindAs(RgBindAs::Sampled);
        Out = R.addOutput("out")
                  .format(rhi::RhiFormat::Rgba8Unorm)
                  .extent2D(64, 64)
                  .usage(rhi::RhiTextureUsage::ColorAttachment | rhi::RhiTextureUsage::Sampled);
    }
    void execute(RenderContext&, const RenderData&) override {}
};

} // namespace

TEST(AliasingTests, PerGroupAcquireReleaseListsPopulate)
{
    RenderGraph Rg("alias_probe");
    auto* A = Rg.addPass<WritePass>("A");
    auto* B = Rg.addPass<ReadPass>("B");
    auto* C = Rg.addPass<ReadPass>("C");
    Rg.connect(A->Out, B->In);
    Rg.connect(B->Out, C->In);
    Rg.markOutput(C->Out);
    ASSERT_TRUE(Rg.compile(nullptr).isOk());

    const auto* S = Rg.debugSchedule();
    ASSERT_NE(S, nullptr);
    // Single graphics group -- every transient both starts and ends inside it.
    ASSERT_EQ(S->Groups.size(), 1u);
    const auto& G = S->Groups[0];
    EXPECT_FALSE(G.AcquireAtStart.empty());
    EXPECT_FALSE(G.ReleaseAfter.empty());
    EXPECT_EQ(G.AcquireAtStart.size(), G.ReleaseAfter.size());
}

TEST(AliasingTests, DebugNameCarriesThroughToCompiledGraph)
{
    RenderGraph Rg("name_probe");
    auto* P = Rg.addPass<WritePass>("gbuffer");
    Rg.markOutput(P->Out);
    ASSERT_TRUE(Rg.compile(nullptr).isOk());

    const auto* Cg = Rg.debugCompiledGraph();
    ASSERT_NE(Cg, nullptr);
    ASSERT_EQ(Cg->LogicalResources.size(), 1u);
    EXPECT_EQ(Cg->LogicalResources[0].DebugName, std::string("gbuffer.out"));
}
