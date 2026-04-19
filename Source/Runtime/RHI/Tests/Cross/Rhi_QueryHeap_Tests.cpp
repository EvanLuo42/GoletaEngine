/// @file
/// @brief Query heap across backends (timestamp only for MVP).

#include "Common/RhiTestFixture.h"

using namespace goleta::rhi::tests;

GPU_TEST(QueryHeap, TimestampCreates, BackendMask::All)
{
    RhiQueryHeapDesc D{};
    D.Kind  = RhiQueryKind::Timestamp;
    D.Count = 16;
    auto Q = F.Device->createQueryHeap(D);
    ASSERT_TRUE(Q);
    EXPECT_EQ(Q->queryKind(), RhiQueryKind::Timestamp);
    EXPECT_EQ(Q->capacity(), 16u);
}
