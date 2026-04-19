/// @file
/// @brief Sampler creation across backends.

#include "Common/RhiTestFixture.h"

using namespace goleta::rhi::tests;

GPU_TEST(Sampler, LinearClampSucceeds, BackendMask::All)
{
    RhiSamplerDesc S{};
    S.AddressU = RhiSamplerAddressMode::ClampToEdge;
    S.AddressV = RhiSamplerAddressMode::ClampToEdge;
    S.AddressW = RhiSamplerAddressMode::ClampToEdge;
    auto Smp = F.Device->createSampler(S);
    ASSERT_TRUE(Smp);
}

GPU_TEST(Sampler, AnisotropicMaxAniso, BackendMask::All)
{
    RhiSamplerDesc S{};
    S.MaxAnisotropy = 16;
    auto Smp = F.Device->createSampler(S);
    ASSERT_TRUE(Smp);
}
