/// @file
/// @brief Buffer creation, mapping, and GPU copy round-trip across backends.

#include <cstring>
#include <vector>

#include "Common/RhiTestFixture.h"
#include "Common/RhiTestHelpers.h"

using namespace goleta::rhi::tests;

namespace
{
Rc<IRhiBuffer> makeBuf(IRhiDevice* D, uint64_t Bytes, RhiMemoryLocation Loc, RhiBufferUsage Usage,
                       const char* Name = nullptr)
{
    RhiBufferDesc B{};
    B.SizeBytes = Bytes;
    B.Usage     = Usage;
    B.Location  = Loc;
    B.DebugName = Name;
    return D->createBuffer(B);
}
} // namespace

GPU_TEST(Buffer, CreateUploadMap, BackendMask::All)
{
    auto Buf = makeBuf(F.Device, 256, RhiMemoryLocation::Upload, RhiBufferUsage::CopySource, "UploadBuf");
    ASSERT_TRUE(Buf);
    auto* Ptr = static_cast<uint32_t*>(Buf->map(0, 256));
    ASSERT_NE(Ptr, nullptr);
    for (uint32_t I = 0; I < 64; ++I) Ptr[I] = I + 1;
    Buf->unmap();
}

GPU_TEST(Buffer, DeviceLocalMapReturnsNull, BackendMask::All)
{
    auto Buf = makeBuf(F.Device, 128, RhiMemoryLocation::DeviceLocal, RhiBufferUsage::StorageBuffer);
    ASSERT_TRUE(Buf);
    EXPECT_EQ(Buf->map(0, 128), nullptr);
}

GPU_TEST(Buffer, DefaultCopyCycle, GpuOnly)
{
    constexpr uint64_t N = 16;
    std::vector<uint32_t> Expected(N);
    for (uint32_t I = 0; I < N; ++I) Expected[I] = 0xDEADBE00u + I;

    auto Up = makeBuf(F.Device, N * 4, RhiMemoryLocation::Upload, RhiBufferUsage::CopySource);
    ASSERT_TRUE(Up);
    std::memcpy(Up->map(0, N * 4), Expected.data(), N * 4);
    Up->unmap();

    auto Def = makeBuf(F.Device, N * 4, RhiMemoryLocation::DeviceLocal,
                        RhiBufferUsage::CopyDest | RhiBufferUsage::CopySource | RhiBufferUsage::StorageBuffer);
    ASSERT_TRUE(Def);

    auto Pool = F.Device->createCommandPool(RhiQueueKind::Graphics);
    auto Cl   = Pool->allocate();
    Cl->begin();
    Cl->copyBuffer(Def.get(), 0, Up.get(), 0, N * 4);
    Cl->end();
    submitAndWait(F.Device, F.Gfx, Cl.get());

    checkBufferEquals<uint32_t>(F.Device, F.Gfx, Def.get(), std::span<const uint32_t>(Expected.data(), N));
}

GPU_TEST(Buffer, StorageHandleAllocatesBindlessSlot, GpuOnly)
{
    auto Buf = makeBuf(F.Device, 4096, RhiMemoryLocation::DeviceLocal, RhiBufferUsage::StorageBuffer);
    ASSERT_TRUE(Buf);
    EXPECT_NE(Buf->uavHandle().Index, InvalidBindlessIndex);
}
