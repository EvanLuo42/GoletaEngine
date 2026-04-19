/// @file
/// @brief Bindless descriptor heap + descriptor-set tests across backends.

#include <array>

#include "Common/RhiTestFixture.h"

using namespace goleta::rhi::tests;

GPU_TEST(DescriptorHeap, BindlessHeapNonNull, BackendMask::All)
{
    auto* H = F.Device->bindlessHeap();
    ASSERT_NE(H, nullptr);
    EXPECT_GT(H->capacity(), 0u);
    EXPECT_GT(H->samplerCapacity(), 0u);
}

GPU_TEST(DescriptorSet, CreateThreeBindingLayout, BackendMask::All)
{
    std::array<RhiDescriptorBinding, 3> Bindings{};
    Bindings[0] = {0, RhiDescriptorKind::ConstantBuffer, 1, RhiShaderStageMask::All, false};
    Bindings[1] = {1, RhiDescriptorKind::SampledTexture, 1, RhiShaderStageMask::All, false};
    Bindings[2] = {2, RhiDescriptorKind::Sampler,        1, RhiShaderStageMask::All, false};

    RhiDescriptorSetLayoutDesc D{};
    D.Bindings     = Bindings.data();
    D.BindingCount = 3;
    auto Layout = F.Device->createDescriptorSetLayout(D);
    ASSERT_TRUE(Layout);
    auto Set = F.Device->createDescriptorSet(Layout.get());
    ASSERT_TRUE(Set);
    EXPECT_EQ(Set->layout(), Layout.get());
}

GPU_TEST(DescriptorSet, WritesDontCrash, BackendMask::All)
{
    std::array<RhiDescriptorBinding, 1> Bindings{};
    Bindings[0] = {0, RhiDescriptorKind::ConstantBuffer, 1, RhiShaderStageMask::All, false};
    RhiDescriptorSetLayoutDesc D{};
    D.Bindings     = Bindings.data();
    D.BindingCount = 1;
    auto Layout = F.Device->createDescriptorSetLayout(D);
    auto Set    = F.Device->createDescriptorSet(Layout.get());

    RhiBufferDesc B{};
    B.SizeBytes = 256; B.Usage = RhiBufferUsage::ConstantBuffer; B.Location = RhiMemoryLocation::Upload;
    auto Buf = F.Device->createBuffer(B);
    ASSERT_TRUE(Buf);

    Set->writeConstantBuffer(0, 0, Buf.get(), 0, 256);
    SUCCEED();
}
