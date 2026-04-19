/// @file
/// @brief IRhiResource::setDebugName round-trip. Ported from RHINullBackendTests::ObjectNaming
///        to verify every backend preserves and clears the debug name correctly.

#include "Common/RhiTestFixture.h"

using namespace goleta::rhi::tests;

GPU_TEST(ObjectNaming, BufferDebugNameRoundTrip, BackendMask::All)
{
    RhiBufferDesc BD{};
    BD.SizeBytes = 16;
    BD.Usage     = RhiBufferUsage::StorageBuffer;
    BD.DebugName = "TestBuffer";
    auto Buf = F.Device->createBuffer(BD);
    ASSERT_TRUE(Buf);
    EXPECT_STREQ(Buf->debugName(), "TestBuffer");
    Buf->setDebugName("RenamedBuffer");
    EXPECT_STREQ(Buf->debugName(), "RenamedBuffer");
    Buf->setDebugName(nullptr);
    EXPECT_STREQ(Buf->debugName(), "");
}
