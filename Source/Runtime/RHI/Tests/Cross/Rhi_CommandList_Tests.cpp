/// @file
/// @brief Command-list recording + enhanced-barrier copy round-trip.

#include "Common/RhiTestFixture.h"
#include "Common/RhiTestHelpers.h"

using namespace goleta::rhi::tests;

GPU_TEST(CommandList, BeginEndClose, BackendMask::All)
{
    auto Pool = F.Device->createCommandPool(RhiQueueKind::Graphics);
    auto Cl   = Pool->allocate();
    ASSERT_TRUE(Cl);
    Cl->begin();
    Cl->insertDebugMarker("hello", 0xFF0000FF);
    Cl->beginDebugScope("scope", 0xFF00FF00);
    Cl->endDebugScope();
    Cl->end();
}

GPU_TEST(CommandList, CopyBufferRoundTripWithBarrier, GpuOnly)
{
    RhiBufferDesc UpDesc{};
    UpDesc.SizeBytes = 64;
    UpDesc.Usage     = RhiBufferUsage::CopySource;
    UpDesc.Location  = RhiMemoryLocation::Upload;
    auto Up = F.Device->createBuffer(UpDesc);
    ASSERT_TRUE(Up);
    auto* P = static_cast<uint32_t*>(Up->map(0, 64));
    ASSERT_NE(P, nullptr);
    for (uint32_t I = 0; I < 16; ++I) P[I] = 0xA0000000u | I;
    Up->unmap();

    RhiBufferDesc DefDesc = UpDesc;
    DefDesc.Location  = RhiMemoryLocation::DeviceLocal;
    DefDesc.Usage     = RhiBufferUsage::CopyDest | RhiBufferUsage::CopySource;
    auto Def = F.Device->createBuffer(DefDesc);

    RhiBufferDesc ReadDesc = UpDesc;
    ReadDesc.Usage    = RhiBufferUsage::CopyDest;
    ReadDesc.Location = RhiMemoryLocation::Readback;
    auto Read = F.Device->createBuffer(ReadDesc);

    auto Pool = F.Device->createCommandPool(RhiQueueKind::Graphics);
    auto Cl   = Pool->allocate();
    Cl->begin();
    Cl->copyBuffer(Def.get(), 0, Up.get(), 0, 64);

    RhiBufferBarrier BB{};
    BB.Buffer    = Def.get();
    BB.SrcStages = RhiPipelineStage::Copy;
    BB.DstStages = RhiPipelineStage::Copy;
    BB.SrcAccess = RhiAccess::CopyWrite;
    BB.DstAccess = RhiAccess::CopyRead;
    RhiBarrierGroup G{};
    G.Buffers     = &BB;
    G.BufferCount = 1;
    Cl->barriers(G);

    Cl->copyBuffer(Read.get(), 0, Def.get(), 0, 64);
    Cl->end();
    submitAndWait(F.Device, F.Gfx, Cl.get());

    auto* R = static_cast<uint32_t*>(Read->map(0, 64));
    ASSERT_NE(R, nullptr);
    for (uint32_t I = 0; I < 16; ++I)
    {
        SCOPED_TRACE("i=" + std::to_string(I));
        EXPECT_EQ(R[I], 0xA0000000u | I);
    }
    Read->unmap();
}

GPU_TEST(CommandList, PoolResetRecyclesLists, BackendMask::All)
{
    auto Pool = F.Device->createCommandPool(RhiQueueKind::Graphics);
    auto Cl1  = Pool->allocate();
    Cl1->begin(); Cl1->end();
    submitAndWait(F.Device, F.Gfx, Cl1.get());
    Pool->reset();
    auto Cl2 = Pool->allocate();
    ASSERT_TRUE(Cl2);
    Cl2->begin(); Cl2->end();
    submitAndWait(F.Device, F.Gfx, Cl2.get());
}
