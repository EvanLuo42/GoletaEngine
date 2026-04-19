/// @file
/// @brief Feature / format-caps probes across backends.

#include "Common/RhiTestFixture.h"

using namespace goleta::rhi::tests;

GPU_TEST(Features, OptionalFeaturesDisabledOnMvp, BackendMask::All)
{
    const auto& FF = F.Device->features();
    EXPECT_FALSE(FF.RayTracing.Supported);
    EXPECT_FALSE(FF.MeshShading.Supported);
    EXPECT_FALSE(FF.WorkGraphs.Supported);
    EXPECT_FALSE(FF.SparseBinding.Supported);
}

GPU_TEST(Features, BindlessSupportedOnGpu, GpuOnly)
{
    const auto& FF = F.Device->features();
    EXPECT_TRUE(FF.BindlessResources.Supported);
    EXPECT_GT(FF.BindlessResources.MaxSrvDescriptors, 1024u);
    EXPECT_GE(FF.BindlessResources.MaxSamplers, 1024u);
}

GPU_TEST(Features, FormatCaps_Rgba8Unorm, GpuOnly)
{
    const auto Caps = F.Device->features().formatCaps(RhiFormat::Rgba8Unorm);
    EXPECT_TRUE(anyOf(Caps, RhiFormatCaps::Sampled));
    EXPECT_TRUE(anyOf(Caps, RhiFormatCaps::ColorAttachment));
}

GPU_TEST(Features, FormatCaps_D32Float, GpuOnly)
{
    const auto Caps = F.Device->features().formatCaps(RhiFormat::D32Float);
    EXPECT_TRUE(anyOf(Caps, RhiFormatCaps::DepthAttachment));
}

GPU_TEST(Features, FormatCaps_Unknown, BackendMask::All)
{
    EXPECT_EQ(F.Device->features().formatCaps(RhiFormat::Unknown), RhiFormatCaps::None);
}
