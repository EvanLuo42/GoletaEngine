#include <gtest/gtest.h>

#include "RHIFeatures.h"

using namespace goleta::rhi;

TEST(RHIFeaturesTests, DefaultConstructedIsAllZero)
{
    RhiDeviceFeatures F{};
    EXPECT_FALSE(F.RayTracing.Supported);
    EXPECT_FALSE(F.MeshShading.Supported);
    EXPECT_FALSE(F.VariableRateShading.Supported);
    EXPECT_FALSE(F.WorkGraphs.Supported);
    EXPECT_FALSE(F.BindlessResources.Supported);
    EXPECT_FALSE(F.Core.TimelineSemaphore);
    EXPECT_EQ(F.MaxTextureDimension2D, 0u);
    EXPECT_EQ(F.MaxPushConstantBytes, 0u);
    EXPECT_EQ(F.FormatCapsFn, nullptr);
}

TEST(RHIFeaturesTests, FormatCapsWithoutFnReturnsNone)
{
    RhiDeviceFeatures F{};
    EXPECT_EQ(static_cast<uint32_t>(F.formatCaps(RhiFormat::Rgba8Unorm)), static_cast<uint32_t>(RhiFormatCaps::None));
}

TEST(RHIFeaturesTests, FormatCapsFnIsInvokedWhenSet)
{
    RhiDeviceFeatures F{};
    F.FormatCapsFn = [](RhiFormat Fmt)
    {
        return Fmt == RhiFormat::Rgba8Unorm ? (RhiFormatCaps::Sampled | RhiFormatCaps::ColorAttachment)
                                            : RhiFormatCaps::None;
    };
    const auto Caps = F.formatCaps(RhiFormat::Rgba8Unorm);
    EXPECT_TRUE(anyOf(Caps, RhiFormatCaps::Sampled));
    EXPECT_TRUE(anyOf(Caps, RhiFormatCaps::ColorAttachment));
    EXPECT_FALSE(anyOf(Caps, RhiFormatCaps::Storage));

    EXPECT_EQ(static_cast<uint32_t>(F.formatCaps(RhiFormat::Rgba32Float)), static_cast<uint32_t>(RhiFormatCaps::None));
}
