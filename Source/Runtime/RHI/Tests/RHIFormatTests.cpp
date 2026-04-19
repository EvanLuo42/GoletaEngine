#include <gtest/gtest.h>

#include "RHIFormat.h"

using namespace goleta::rhi;

TEST(RHIFormatTests, SizeOfCommonFormats)
{
    EXPECT_EQ(formatSizeBytes(RhiFormat::Rgba8Unorm), 4u);
    EXPECT_EQ(formatSizeBytes(RhiFormat::Rgba16Float), 8u);
    EXPECT_EQ(formatSizeBytes(RhiFormat::Rgba32Float), 16u);
    EXPECT_EQ(formatSizeBytes(RhiFormat::R8Unorm), 1u);
    EXPECT_EQ(formatSizeBytes(RhiFormat::R11G11B10Float), 4u);
    EXPECT_EQ(formatSizeBytes(RhiFormat::D32Float), 4u);
}

TEST(RHIFormatTests, UnknownAndOutOfRangeReturnZero)
{
    EXPECT_EQ(formatSizeBytes(RhiFormat::Unknown), 0u);
    // Casting past the end must still be safe.
    const auto Oor = static_cast<RhiFormat>(static_cast<uint32_t>(RhiFormat::Count) + 100);
    EXPECT_EQ(formatSizeBytes(Oor), 0u);
}

TEST(RHIFormatTests, DepthAndStencilClassification)
{
    EXPECT_TRUE(formatIsDepth(RhiFormat::D16Unorm));
    EXPECT_TRUE(formatIsDepth(RhiFormat::D32Float));
    EXPECT_TRUE(formatIsDepth(RhiFormat::D24UnormS8Uint));
    EXPECT_FALSE(formatIsDepth(RhiFormat::Rgba8Unorm));

    EXPECT_TRUE(formatIsStencil(RhiFormat::D24UnormS8Uint));
    EXPECT_TRUE(formatIsStencil(RhiFormat::D32FloatS8Uint));
    EXPECT_FALSE(formatIsStencil(RhiFormat::D32Float));
    EXPECT_FALSE(formatIsStencil(RhiFormat::Rgba8Unorm));
}

TEST(RHIFormatTests, SrgbClassification)
{
    EXPECT_TRUE(formatIsSrgb(RhiFormat::Rgba8UnormSrgb));
    EXPECT_TRUE(formatIsSrgb(RhiFormat::Bc7RgbaUnormSrgb));
    EXPECT_FALSE(formatIsSrgb(RhiFormat::Rgba8Unorm));
    EXPECT_FALSE(formatIsSrgb(RhiFormat::Bc7RgbaUnorm));
}

TEST(RHIFormatTests, BlockCompressedFormats)
{
    EXPECT_TRUE(formatIsBlockCompressed(RhiFormat::Bc1RgbUnorm));
    EXPECT_TRUE(formatIsBlockCompressed(RhiFormat::Bc7RgbaUnormSrgb));
    EXPECT_FALSE(formatIsBlockCompressed(RhiFormat::Rgba8Unorm));

    uint32_t W = 0, H = 0;
    formatBlockExtent(RhiFormat::Bc1RgbUnorm, W, H);
    EXPECT_EQ(W, 4u);
    EXPECT_EQ(H, 4u);

    formatBlockExtent(RhiFormat::Rgba8Unorm, W, H);
    EXPECT_EQ(W, 1u);
    EXPECT_EQ(H, 1u);
}
