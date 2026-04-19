/// @file
/// @brief Cross-backend compute pipeline: Slang → DXIL|SPIR-V → PSO → dispatch → readback.

#include <vector>

#include "Common/RhiSlangCompiler.h"
#include "Common/RhiTestFixture.h"
#include "Common/RhiTestHelpers.h"

using namespace goleta::rhi::tests;

GPU_TEST(ComputePipeline, TrivialDispatchBindless, GpuOnly)
{
    if (!slangRuntimeAvailable())
        GTEST_SKIP() << "slang prebuilt unavailable";

    auto Compiled = compileSlangForBackend(F.GetParam(), slangShaderPath("ComputeTrivial.slang"), "computeMain");
    ASSERT_TRUE(Compiled.isOk()) << "slang compile failed: " << Compiled.Diagnostics;

    RhiShaderModuleDesc SM{};
    SM.Kind         = shaderKindFor(F.GetParam());
    SM.Stage        = RhiShaderStage::Compute;
    SM.Bytecode     = Compiled.Bytecode.data();
    SM.BytecodeSize = Compiled.Bytecode.size();
    SM.EntryPoint   = "computeMain";
    auto Shader     = F.Device->createShaderModule(SM);
    ASSERT_TRUE(Shader);

    RhiComputePipelineDesc PD{};
    PD.ComputeShader            = Shader.get();
    PD.Bindings.UseBindlessHeap = true;
    auto Pso = F.Device->createComputePipeline(PD);
    ASSERT_TRUE(Pso);

    RhiBufferDesc B{};
    B.SizeBytes       = 64 * sizeof(uint32_t);
    B.Usage           = RhiBufferUsage::StorageBuffer | RhiBufferUsage::CopySource;
    B.Location        = RhiMemoryLocation::DeviceLocal;
    B.StructureStride = sizeof(uint32_t);
    B.DebugName       = "TrivialOut";
    auto Out = F.Device->createBuffer(B);
    ASSERT_TRUE(Out);

    auto Pool = F.Device->createCommandPool(RhiQueueKind::Graphics);
    auto Cl   = Pool->allocate();
    Cl->begin();
    Cl->setComputePipeline(Pso.get());
    const uint32_t OutIdx = Out->uavHandle().Index;
    Cl->setRootConstants(&OutIdx, sizeof(uint32_t), 0);
    Cl->dispatch(1, 1, 1);
    Cl->end();
    submitAndWait(F.Device, F.Gfx, Cl.get());

    std::vector<uint32_t> Expected(64);
    for (uint32_t I = 0; I < 64; ++I) Expected[I] = I * 2;
    checkBufferEquals<uint32_t>(F.Device, F.Gfx, Out.get(), std::span<const uint32_t>(Expected.data(), 64));
}
