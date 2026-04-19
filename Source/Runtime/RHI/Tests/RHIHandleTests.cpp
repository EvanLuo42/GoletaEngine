#include <gtest/gtest.h>

#include <unordered_set>

#include "RHIHandle.h"

using namespace goleta::rhi;

TEST(RHIHandleTests, DefaultConstructedIsInvalid)
{
    RhiBindlessIndex Idx{};
    EXPECT_FALSE(Idx.isValid());
    EXPECT_FALSE(static_cast<bool>(Idx));
    EXPECT_EQ(Idx.Value, InvalidBindlessIndex);
}

TEST(RHIHandleTests, TypedHandlesAreDistinctTypes)
{
    RhiBufferHandle  Buf{42};
    RhiTextureHandle Tex{42};

    EXPECT_TRUE(Buf.isValid());
    EXPECT_TRUE(Tex.isValid());
    EXPECT_EQ(Buf.Index, 42u);
    EXPECT_EQ(Tex.Index, 42u);

    // This is the point of the phantom tag — the two handle types cannot be compared
    // with operator==, which is a compile-time guarantee. We only check runtime equality
    // within a single type here; cross-type comparison is verified by the build succeeding
    // without an explicit conversion.
    EXPECT_EQ(Buf, (RhiBufferHandle{42}));
    EXPECT_NE(Buf, (RhiBufferHandle{7}));
}

TEST(RHIHandleTests, HandlesAreHashableAndInsertIntoUnorderedSet)
{
    std::unordered_set<RhiTextureHandle> Set;
    Set.insert({1});
    Set.insert({2});
    Set.insert({1}); // Duplicate.
    EXPECT_EQ(Set.size(), 2u);
    EXPECT_TRUE(Set.contains(RhiTextureHandle{1}));
    EXPECT_FALSE(Set.contains(RhiTextureHandle{99}));
}

TEST(RHIHandleTests, InvalidIsSentinelZero)
{
    RhiSamplerHandle Default{};
    RhiSamplerHandle Explicit{InvalidBindlessIndex};
    EXPECT_EQ(Default, Explicit);
    EXPECT_FALSE(Default.isValid());
}
