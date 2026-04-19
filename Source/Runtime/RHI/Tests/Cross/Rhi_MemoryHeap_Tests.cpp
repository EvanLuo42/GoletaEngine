/// @file
/// @brief Memory heap + placed resources across backends.

#include "Common/RhiTestFixture.h"

using namespace goleta::rhi::tests;

GPU_TEST(MemoryHeap, CreateDefault, GpuOnly)
{
    RhiHeapDesc H{};
    H.SizeBytes = 64 * 1024 * 1024;
    H.Location  = RhiMemoryLocation::DeviceLocal;
    auto Heap = F.Device->createHeap(H);
    ASSERT_TRUE(Heap);
    EXPECT_EQ(Heap->sizeBytes(), H.SizeBytes);
}

GPU_TEST(MemoryHeap, PlacedBufferDistinctAddresses, GpuOnly)
{
    RhiHeapDesc H{};
    H.SizeBytes  = 4 * 1024 * 1024;
    H.Location   = RhiMemoryLocation::DeviceLocal;
    H.BufferOnly = true;
    auto Heap = F.Device->createHeap(H);
    ASSERT_TRUE(Heap);

    RhiBufferDesc B{};
    B.SizeBytes = 64 * 1024;
    B.Usage     = RhiBufferUsage::StorageBuffer;
    B.Location  = RhiMemoryLocation::DeviceLocal;
    auto B1 = F.Device->createPlacedBuffer(Heap.get(), 0, B);
    auto B2 = F.Device->createPlacedBuffer(Heap.get(), 64 * 1024, B);
    ASSERT_TRUE(B1); ASSERT_TRUE(B2);
    EXPECT_NE(B1->gpuAddress(), B2->gpuAddress());
}
