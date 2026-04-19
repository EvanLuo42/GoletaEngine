#include <gtest/gtest.h>

#include "RHIStructChain.h"

using namespace goleta::rhi;

struct ExtensionA
{
    static constexpr auto kStructType = static_cast<RhiStructType>(0x80110001);
    RhiStructHeader       Header{.sType = kStructType, .pNext = nullptr};
    int                   Payload = 0;
};

struct ExtensionB
{
    static constexpr auto kStructType = static_cast<RhiStructType>(0x80110002);
    RhiStructHeader       Header{.sType = kStructType, .pNext = nullptr};
    float                 Scalar = 0.0f;
};

TEST(RHIStructChainTests, FindsTypeAtHead)
{
    ExtensionA A{};
    A.Payload = 42;

    const auto* Found = findChain<ExtensionA>(&A.Header);
    ASSERT_NE(Found, nullptr);
    EXPECT_EQ(Found->Payload, 42);
}

TEST(RHIStructChainTests, FindsTypeDeeperInChain)
{
    ExtensionB B{};
    B.Scalar = 2.5f;

    ExtensionA A{};
    A.Payload      = 7;
    A.Header.pNext = &B.Header;

    const auto* FoundA = findChain<ExtensionA>(&A.Header);
    const auto* FoundB = findChain<ExtensionB>(&A.Header);
    ASSERT_NE(FoundA, nullptr);
    ASSERT_NE(FoundB, nullptr);
    EXPECT_EQ(FoundA->Payload, 7);
    EXPECT_FLOAT_EQ(FoundB->Scalar, 2.5f);
}

TEST(RHIStructChainTests, ReturnsNullForMissingType)
{
    ExtensionA  A{};
    const auto* FoundB = findChain<ExtensionB>(&A.Header);
    EXPECT_EQ(FoundB, nullptr);
}

TEST(RHIStructChainTests, NullHeadIsTolerated)
{
    const auto* Found = findChain<ExtensionA>(nullptr);
    EXPECT_EQ(Found, nullptr);

    const RhiStructHeader* Link = findChainLink(nullptr, ExtensionA::kStructType);
    EXPECT_EQ(Link, nullptr);
}
