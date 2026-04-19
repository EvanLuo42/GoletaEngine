/// @file
/// @brief Unit tests for RenderGraph typed connect(), markOutput(), setInput().

#include <gtest/gtest.h>

#include "RenderGraph.h"
#include "RenderPass.h"
#include "RenderPassReflection.h"

using namespace goleta;
using namespace goleta::rg;

namespace
{

class GBufferPass : public RenderPass
{
public:
    PassOutput<Texture> Albedo;
    PassOutput<Texture> Normal;

    void reflect(RenderPassReflection& R) override
    {
        Albedo = R.addOutput("albedo")
                     .format(rhi::RhiFormat::Rgba8Unorm)
                     .extent2D(256, 128)
                     .usage(rhi::RhiTextureUsage::ColorAttachment | rhi::RhiTextureUsage::Sampled);
        Normal = R.addOutput("normal")
                     .format(rhi::RhiFormat::Rgba16Float)
                     .extent2D(256, 128)
                     .usage(rhi::RhiTextureUsage::ColorAttachment | rhi::RhiTextureUsage::Sampled);
    }
    void execute(RenderContext&, const RenderData&) override {}
};

class LightingPass : public RenderPass
{
public:
    PassInput<Texture>  Albedo;
    PassInput<Texture>  Normal;
    PassOutput<Texture> Color;

    void reflect(RenderPassReflection& R) override
    {
        Albedo = R.addInput("albedo").bindAs(RgBindAs::Sampled);
        Normal = R.addInput("normal").bindAs(RgBindAs::Sampled);
        Color  = R.addOutput("color")
                     .format(rhi::RhiFormat::Rgba16Float)
                     .extent2D(256, 128)
                     .usage(rhi::RhiTextureUsage::ColorAttachment);
    }
    void execute(RenderContext&, const RenderData&) override {}
};

} // namespace

TEST(RenderGraphConnectTests, TwoPassChainCompiles)
{
    RenderGraph Rg;
    auto* GB  = Rg.addPass<GBufferPass>("gbuffer");
    auto* Lgt = Rg.addPass<LightingPass>("lighting");
    Rg.connect(GB->Albedo, Lgt->Albedo);
    Rg.connect(GB->Normal, Lgt->Normal);
    Rg.markOutput(Lgt->Color);

    auto R = Rg.compile(nullptr);
    ASSERT_TRUE(R.isOk()) << "compile error: " << static_cast<uint32_t>(R.error());
    EXPECT_TRUE(Rg.isCompiled());
}

TEST(RenderGraphConnectTests, UnresolvedInputFails)
{
    RenderGraph   Rg;
    auto*         Lgt = Rg.addPass<LightingPass>("lighting");
    (void)Lgt;
    Rg.markOutput(Lgt->Color);
    auto R = Rg.compile(nullptr);
    ASSERT_TRUE(R.isErr());
    EXPECT_EQ(R.error(), RgError::UnresolvedInput);
}

TEST(RenderGraphConnectTests, CycleIsRejected)
{
    class PingPass : public RenderPass
    {
    public:
        PassInput<Texture>  In;
        PassOutput<Texture> Out;
        void reflect(RenderPassReflection& R) override
        {
            In  = R.addInput("in").bindAs(RgBindAs::Sampled);
            Out = R.addOutput("out")
                      .format(rhi::RhiFormat::Rgba8Unorm)
                      .extent2D(64, 64)
                      .usage(rhi::RhiTextureUsage::ColorAttachment);
        }
        void execute(RenderContext&, const RenderData&) override {}
    };

    RenderGraph Rg;
    auto* A = Rg.addPass<PingPass>("a");
    auto* B = Rg.addPass<PingPass>("b");
    Rg.connect(A->Out, B->In);
    Rg.connect(B->Out, A->In);
    Rg.markOutput(B->Out);
    auto R = Rg.compile(nullptr);
    ASSERT_TRUE(R.isErr());
    EXPECT_EQ(R.error(), RgError::Cycle);
}

TEST(RenderGraphConnectTests, MarkOutputAcceptsPassOutput)
{
    RenderGraph Rg;
    auto* GB = Rg.addPass<GBufferPass>("gbuffer");
    Rg.markOutput(GB->Albedo);
    auto R = Rg.compile(nullptr);
    ASSERT_TRUE(R.isOk());
}
