/// @file
/// @brief Texture / texture-view creation across backends.

#include "Common/RhiTestFixture.h"

using namespace goleta::rhi::tests;

GPU_TEST(Texture, Create2DRgba8, BackendMask::All)
{
    RhiTextureDesc T{};
    T.Dimension = RhiTextureDimension::Tex2D;
    T.Format    = RhiFormat::Rgba8Unorm;
    T.Width     = 256;
    T.Height    = 256;
    T.MipLevels = 1;
    T.Usage     = RhiTextureUsage::Sampled | RhiTextureUsage::ColorAttachment;
    T.DebugName = "TestRt";
    auto Tex    = F.Device->createTexture(T);
    ASSERT_TRUE(Tex);
}

GPU_TEST(Texture, CreateDepthAttachmentOk, BackendMask::All)
{
    RhiTextureDesc T{};
    T.Format    = RhiFormat::D32Float;
    T.Width     = 128;
    T.Height    = 128;
    T.MipLevels = 1;
    T.Usage     = RhiTextureUsage::DepthAttachment;
    T.OptimizedClearValue.UseColor = false;
    T.OptimizedClearValue.Depth    = 1.0f;
    auto Tex = F.Device->createTexture(T);
    ASSERT_TRUE(Tex);
}

GPU_TEST(TextureView, SrvCreatesSuccessfully, BackendMask::All)
{
    RhiTextureDesc T{};
    T.Width = 64; T.Height = 64; T.MipLevels = 3;
    T.Usage = RhiTextureUsage::Sampled | RhiTextureUsage::Storage;
    auto Tex = F.Device->createTexture(T);
    ASSERT_TRUE(Tex);

    RhiTextureViewDesc V{};
    V.Texture       = Tex.get();
    V.Dimension     = RhiTextureDimension::Tex2D;
    V.BaseMipLevel  = 1;
    V.MipLevelCount = 2;
    auto Vw = F.Device->createTextureView(V);
    ASSERT_TRUE(Vw);
}
