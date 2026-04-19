/// @file
/// @brief Native-handle probes. Per-backend expected Kind values are encoded in the fixture via
///        RunMask filters — these tests only instantiate on the backend whose native-handle
///        kind they target.

#include "Common/RhiTestFixture.h"

using namespace goleta::rhi::tests;

GPU_TEST(NativeHandle, D3D12DeviceReturnsD3D12Kind, BackendMask::D3D12)
{
    const auto NH = F.Device->nativeHandle();
    EXPECT_EQ(NH.Kind, RhiNativeHandleKind::D3D12Device);
    EXPECT_NE(NH.Handle, nullptr);
}

GPU_TEST(NativeHandle, D3D12QueueReturnsD3D12Kind, BackendMask::D3D12)
{
    auto Q = F.Device->getQueue(RhiQueueKind::Graphics);
    const auto NH = Q->nativeHandle();
    EXPECT_EQ(NH.Kind, RhiNativeHandleKind::D3D12CommandQueue);
    EXPECT_NE(NH.Handle, nullptr);
}

GPU_TEST(NativeHandle, D3D12FenceReturnsD3D12Kind, BackendMask::D3D12)
{
    auto Fence = F.Device->createFence(0);
    const auto NH = Fence->nativeHandle();
    EXPECT_EQ(NH.Kind, RhiNativeHandleKind::D3D12Fence);
    EXPECT_NE(NH.Handle, nullptr);
}

GPU_TEST(NativeHandle, D3D12BufferReturnsD3D12Kind, BackendMask::D3D12)
{
    RhiBufferDesc B{};
    B.SizeBytes = 64; B.Usage = RhiBufferUsage::StorageBuffer;
    auto Buf = F.Device->createBuffer(B);
    const auto NH = Buf->nativeHandle();
    EXPECT_EQ(NH.Kind, RhiNativeHandleKind::D3D12Resource);
    EXPECT_NE(NH.Handle, nullptr);
}

GPU_TEST(NativeHandle, NullBackendReturnsUnknown, BackendMask::Null)
{
    const auto NH = F.Device->createFence(0)->nativeHandle();
    EXPECT_EQ(NH.Kind, RhiNativeHandleKind::Unknown);
}
