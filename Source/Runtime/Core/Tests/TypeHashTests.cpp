#include <gtest/gtest.h>

#include <cstdint>
#include <string>
#include <vector>

#include "TypeHash.h"

using namespace goleta;

namespace
{
struct Alpha
{
};
struct Beta
{
};

template <class T>
struct Generic
{
};
} // namespace

TEST(TypeHashTests, DistinctTypesProduceDistinctHashes)
{
    EXPECT_NE(typeHash<Alpha>(), typeHash<Beta>());
    EXPECT_NE(typeHash<int>(), typeHash<unsigned int>());
    EXPECT_NE(typeHash<int>(), typeHash<long>());
    EXPECT_NE(typeHash<std::string>(), typeHash<std::vector<int>>());
}

TEST(TypeHashTests, SameTypeYieldsSameHashAcrossCallSites)
{
    const uint64_t A1 = typeHash<Alpha>();
    const uint64_t A2 = typeHash<Alpha>();
    EXPECT_EQ(A1, A2);
}

TEST(TypeHashTests, ConstQualifiersAreDistinct)
{
    EXPECT_NE(typeHash<int>(), typeHash<const int>());
    EXPECT_NE(typeHash<int*>(), typeHash<const int*>());
}

TEST(TypeHashTests, TemplateInstantiationsAreDistinct)
{
    EXPECT_NE(typeHash<Generic<int>>(), typeHash<Generic<float>>());
    EXPECT_NE(typeHash<Generic<Alpha>>(), typeHash<Generic<Beta>>());
}

TEST(TypeHashTests, HashIsCompileTimeConstant)
{
    constexpr uint64_t H = typeHash<Alpha>();
    static_assert(H != 0, "typeHash must produce a non-zero constexpr value");
    EXPECT_NE(H, 0u);
}
