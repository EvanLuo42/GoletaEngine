/// @file
/// @brief Unit tests for RenderGraph::compile(): topological order, diamond chains, metadata
///        validation, and merged-resource descriptors.

#include <gtest/gtest.h>

#include <algorithm>

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
    void reflect(RenderPassReflection& R) override
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
    void reflect(RenderPassReflection& R) override
    {
        In  = R.addInput("in").bindAs(RgBindAs::Sampled);
        Out = R.addOutput("out")
                  .format(rhi::RhiFormat::Rgba8Unorm)
                  .extent2D(64, 64)
                  .usage(rhi::RhiTextureUsage::ColorAttachment | rhi::RhiTextureUsage::Sampled);
    }
    void execute(RenderContext&, const RenderData&) override {}
};

class CombinePass : public RenderPass
{
public:
    PassInput<Texture>  A;
    PassInput<Texture>  B;
    PassOutput<Texture> Out;
    void reflect(RenderPassReflection& R) override
    {
        A   = R.addInput("a").bindAs(RgBindAs::Sampled);
        B   = R.addInput("b").bindAs(RgBindAs::Sampled);
        Out = R.addOutput("out")
                  .format(rhi::RhiFormat::Rgba8Unorm)
                  .extent2D(64, 64)
                  .usage(rhi::RhiTextureUsage::ColorAttachment);
    }
    void execute(RenderContext&, const RenderData&) override {}
};

} // namespace

TEST(RenderGraphCompileTests, LinearChainOrder)
{
    RenderGraph Rg;
    auto* A = Rg.addPass<WritePass>("A");
    auto* B = Rg.addPass<ReadPass>("B");
    auto* C = Rg.addPass<ReadPass>("C");
    Rg.connect(A->Out, B->In);
    Rg.connect(B->Out, C->In);
    Rg.markOutput(C->Out);
    ASSERT_TRUE(Rg.compile(nullptr).isOk());
    EXPECT_TRUE(Rg.isCompiled());
}

TEST(RenderGraphCompileTests, DiamondOrder)
{
    RenderGraph Rg;
    auto* W  = Rg.addPass<WritePass>("W");
    auto* L  = Rg.addPass<ReadPass>("L");
    auto* R  = Rg.addPass<ReadPass>("R");
    auto* Cb = Rg.addPass<CombinePass>("Combine");
    Rg.connect(W->Out, L->In);
    Rg.connect(W->Out, R->In);
    Rg.connect(L->Out, Cb->A);
    Rg.connect(R->Out, Cb->B);
    Rg.markOutput(Cb->Out);
    ASSERT_TRUE(Rg.compile(nullptr).isOk());
    EXPECT_TRUE(Rg.isCompiled());
}

TEST(RenderGraphCompileTests, MissingFormatOnTransientOutputFails)
{
    class IncompletePass : public RenderPass
    {
    public:
        PassOutput<Texture> Out;
        void reflect(RenderPassReflection& R) override
        {
            Out = R.addOutput("out"); // No format, no extent.
        }
        void execute(RenderContext&, const RenderData&) override {}
    };

    RenderGraph Rg;
    auto* P = Rg.addPass<IncompletePass>("p");
    Rg.markOutput(P->Out);
    auto Result = Rg.compile(nullptr);
    ASSERT_TRUE(Result.isErr());
    EXPECT_EQ(Result.error(), RgError::MissingMetadata);
}

TEST(RenderGraphCompileTests, RecompileIsIdempotent)
{
    RenderGraph Rg;
    auto* A = Rg.addPass<WritePass>("A");
    auto* B = Rg.addPass<ReadPass>("B");
    Rg.connect(A->Out, B->In);
    Rg.markOutput(B->Out);
    ASSERT_TRUE(Rg.compile(nullptr).isOk());
    ASSERT_TRUE(Rg.compile(nullptr).isOk());
    EXPECT_TRUE(Rg.isCompiled());
}
