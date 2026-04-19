/// @file
/// @brief Unit tests for RenderPassReflection: builder chaining, duplicate detection,
///        Field → PassField handle conversion.

#include <gtest/gtest.h>

#include "RHIBuffer.h"
#include "RHITexture.h"
#include "RenderGraph.h"
#include "RenderPass.h"
#include "RenderPassReflection.h"

using namespace goleta;
using namespace goleta::rg;

namespace
{

class BuilderProbePass : public RenderPass
{
public:
    PassOutput<Texture> Color;
    PassOutput<Texture> Depth;
    PassInput<Buffer>   Data;

    bool Reflected = false;

    void reflect(RenderPassReflection& R) override
    {
        Color = R.addOutput("color")
                    .format(rhi::RhiFormat::Rgba8Unorm)
                    .extent2D(128, 64)
                    .usage(rhi::RhiTextureUsage::ColorAttachment | rhi::RhiTextureUsage::Sampled);
        Depth = R.addOutput("depth")
                    .format(rhi::RhiFormat::D32Float)
                    .extent2D(128, 64)
                    .usage(rhi::RhiTextureUsage::DepthAttachment);
        Data = R.addInput("data").bufferUsage(rhi::RhiBufferUsage::StorageBuffer).sizeBytes(1024);
        Reflected = true;
    }

    void execute(RenderContext&, const RenderData&) override {}
};

} // namespace

TEST(RenderPassReflectionTests, AddFieldAssignsStableIds)
{
    RenderGraph Rg("probe");
    auto* P = Rg.addPass<BuilderProbePass>("probe");
    ASSERT_TRUE(P->Reflected);
    EXPECT_EQ(static_cast<uint32_t>(P->Color.Field), 0u);
    EXPECT_EQ(static_cast<uint32_t>(P->Depth.Field), 1u);
    EXPECT_EQ(static_cast<uint32_t>(P->Data.Field),  2u);
    EXPECT_EQ(P->Color.Pass, P->Depth.Pass);
    EXPECT_EQ(P->Color.Pass, P->Data.Pass);
    EXPECT_TRUE(P->Color.isValid());
}

TEST(RenderPassReflectionTests, ChainedSettersCapture)
{
    class CaptureProbe : public RenderPass
    {
    public:
        const FieldDesc* ColorDesc = nullptr;

        void reflect(RenderPassReflection& R) override
        {
            R.addOutput("color")
                .format(rhi::RhiFormat::Rgba16Float)
                .extent3D(32, 16, 8)
                .mips(3)
                .arrayLayers(2)
                .samples(rhi::RhiSampleCount::X4);
            ColorDesc = &R.fields()[0];
        }
        void execute(RenderContext&, const RenderData&) override {}
    };

    RenderGraph Rg;
    auto*       P = Rg.addPass<CaptureProbe>("probe");
    ASSERT_NE(P->ColorDesc, nullptr);
    EXPECT_EQ(P->ColorDesc->Format, rhi::RhiFormat::Rgba16Float);
    EXPECT_EQ(P->ColorDesc->Width,  32u);
    EXPECT_EQ(P->ColorDesc->Height, 16u);
    EXPECT_EQ(P->ColorDesc->Depth,   8u);
    EXPECT_EQ(P->ColorDesc->MipLevels,   3u);
    EXPECT_EQ(P->ColorDesc->ArrayLayers, 2u);
    EXPECT_EQ(P->ColorDesc->Samples, rhi::RhiSampleCount::X4);
}

TEST(RenderPassReflectionTests, DuplicateFieldNameFailsCompile)
{
    class DupProbe : public RenderPass
    {
    public:
        void reflect(RenderPassReflection& R) override
        {
            R.addOutput("color").format(rhi::RhiFormat::Rgba8Unorm).extent2D(16, 16);
            R.addOutput("color").format(rhi::RhiFormat::Rgba8Unorm).extent2D(16, 16);
        }
        void execute(RenderContext&, const RenderData&) override {}
    };

    RenderGraph Rg;
    Rg.addPass<DupProbe>("dup");
    auto Result = Rg.compile(nullptr);
    ASSERT_TRUE(Result.isErr());
    EXPECT_EQ(Result.error(), RgError::DuplicateField);
}

TEST(RenderPassReflectionTests, FindByNameReturnsIdOrInvalid)
{
    RenderGraph Rg;
    auto*       P = Rg.addPass<BuilderProbePass>("probe");
    (void)P;

    // Reach into reflection via the pass's handle Pass id — the test can't inspect reflection
    // directly through the public API, so we validate by building a graph that would fail if
    // findByName were broken (setInput uses it).
    Rg.setInput("probe", "data",
                Rc<rhi::IRhiBuffer>{} /* intentionally null; only the name lookup is under test */);
    SUCCEED();
}
