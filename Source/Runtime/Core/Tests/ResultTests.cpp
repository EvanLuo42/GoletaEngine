#include <gtest/gtest.h>

#include <string>

#include "Result.h"

using namespace goleta;

namespace
{
enum class TestError
{
    NotFound,
    BadArg,
};
}

TEST(ResultTests, OkCarriesValue)
{
    Result<int, TestError> R = Ok{42};
    EXPECT_TRUE(R.isOk());
    EXPECT_FALSE(R.isErr());
    EXPECT_TRUE(static_cast<bool>(R));
    EXPECT_EQ(R.value(), 42);
}

TEST(ResultTests, ErrCarriesError)
{
    Result<int, TestError> R = Err{TestError::NotFound};
    EXPECT_FALSE(R.isOk());
    EXPECT_TRUE(R.isErr());
    EXPECT_EQ(R.error(), TestError::NotFound);
}

TEST(ResultTests, ValueOrFallsBackOnErr)
{
    Result<int, TestError> Good = Ok{7};
    Result<int, TestError> Bad  = Err{TestError::BadArg};
    EXPECT_EQ(Good.valueOr(0), 7);
    EXPECT_EQ(Bad.valueOr(99), 99);
}

TEST(ResultTests, MoveOnlyPayload)
{
    Result<std::string, TestError> R = Ok{std::string("hello")};
    ASSERT_TRUE(R.isOk());
    std::string Moved = std::move(R).value();
    EXPECT_EQ(Moved, "hello");
}

TEST(ResultTests, VoidSuccessDefaultConstructs)
{
    Result<void, TestError> R;
    EXPECT_TRUE(R.isOk());
    EXPECT_FALSE(R.isErr());
}

TEST(ResultTests, VoidErrCarriesError)
{
    Result<void, TestError> R = Err{TestError::NotFound};
    ASSERT_TRUE(R.isErr());
    EXPECT_EQ(R.error(), TestError::NotFound);
}

TEST(ResultTests, ConvertingConstructorFromNarrowerType)
{
    Result<uint32_t, TestError> R = Ok{42}; // int literal -> uint32_t
    ASSERT_TRUE(R.isOk());
    EXPECT_EQ(R.value(), 42u);
}
