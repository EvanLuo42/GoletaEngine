/// @file
/// @brief Unit tests for Vec2 / Vec3 / Vec4.

#include "MathTestHelpers.h"

using namespace goleta::math;
using goleta::math::testing::nearFloat;
using goleta::math::testing::nearVec3;
using goleta::math::testing::nearVec4;

TEST(Vec4Test, ComponentwiseConstructionAndAccess)
{
    const Vec4 V(1.0f, 2.0f, 3.0f, 4.0f);
    EXPECT_FLOAT_EQ(V.x(), 1.0f);
    EXPECT_FLOAT_EQ(V.y(), 2.0f);
    EXPECT_FLOAT_EQ(V.z(), 3.0f);
    EXPECT_FLOAT_EQ(V.w(), 4.0f);
}

TEST(Vec4Test, AddSubMulScalar)
{
    const Vec4 A(1.0f, 2.0f, 3.0f, 4.0f);
    const Vec4 B(10.0f, 20.0f, 30.0f, 40.0f);
    EXPECT_TRUE(nearVec4(A + B, Vec4(11.0f, 22.0f, 33.0f, 44.0f)));
    EXPECT_TRUE(nearVec4(B - A, Vec4(9.0f, 18.0f, 27.0f, 36.0f)));
    EXPECT_TRUE(nearVec4(A * 2.0f, Vec4(2.0f, 4.0f, 6.0f, 8.0f)));
    EXPECT_TRUE(nearVec4(2.0f * A, Vec4(2.0f, 4.0f, 6.0f, 8.0f)));
}

TEST(Vec4Test, DotProduct)
{
    const Vec4 A(1.0f, 2.0f, 3.0f, 4.0f);
    const Vec4 B(5.0f, 6.0f, 7.0f, 8.0f);
    EXPECT_FLOAT_EQ(dot(A, B), 1 * 5 + 2 * 6 + 3 * 7 + 4 * 8);
}

TEST(Vec4Test, LengthAndNormalize)
{
    const Vec4 V(3.0f, 0.0f, 4.0f, 0.0f);
    EXPECT_FLOAT_EQ(length(V), 5.0f);
    EXPECT_TRUE(nearFloat(length(normalize(V)), 1.0f));
}

TEST(Vec4Test, Lerp)
{
    const Vec4 A(0.0f, 0.0f, 0.0f, 0.0f);
    const Vec4 B(10.0f, 20.0f, 30.0f, 40.0f);
    EXPECT_TRUE(nearVec4(lerp(A, B, 0.0f), A));
    EXPECT_TRUE(nearVec4(lerp(A, B, 1.0f), B));
    EXPECT_TRUE(nearVec4(lerp(A, B, 0.5f), Vec4(5.0f, 10.0f, 15.0f, 20.0f)));
}

TEST(Vec3Test, CrossProductRightHanded)
{
    EXPECT_TRUE(nearVec3(cross(Vec3::unitX(), Vec3::unitY()), Vec3::unitZ()));
    EXPECT_TRUE(nearVec3(cross(Vec3::unitY(), Vec3::unitZ()), Vec3::unitX()));
    EXPECT_TRUE(nearVec3(cross(Vec3::unitZ(), Vec3::unitX()), Vec3::unitY()));
}

TEST(Vec3Test, DotAndLength)
{
    const Vec3 A(1.0f, 2.0f, 2.0f);
    EXPECT_FLOAT_EQ(dot(A, A), 9.0f);
    EXPECT_FLOAT_EQ(length(A), 3.0f);
}

TEST(Vec3Test, Normalize)
{
    const Vec3 V(0.0f, 3.0f, 4.0f);
    const Vec3 N = normalize(V);
    EXPECT_TRUE(nearFloat(length(N), 1.0f));
    EXPECT_TRUE(nearVec3(N, Vec3(0.0f, 0.6f, 0.8f)));
}

TEST(Vec3Test, ReflectAcrossHorizontalPlane)
{
    const Vec3 Incident(1.0f, -1.0f, 0.0f);
    const Vec3 Normal = Vec3::unitY();
    EXPECT_TRUE(nearVec3(reflect(Incident, Normal), Vec3(1.0f, 1.0f, 0.0f)));
}

TEST(Vec2Test, BasicOps)
{
    const Vec2 A(3.0f, 4.0f);
    EXPECT_FLOAT_EQ(length(A), 5.0f);
    EXPECT_FLOAT_EQ(dot(A, Vec2(1.0f, 0.0f)), 3.0f);
    EXPECT_TRUE(nearFloat(length(normalize(A)), 1.0f));
}

TEST(Vec4Test, SplatConstructor)
{
    EXPECT_TRUE(nearVec4(Vec4(3.0f), Vec4(3.0f, 3.0f, 3.0f, 3.0f)));
}

TEST(Vec4Test, StaticFactories)
{
    EXPECT_TRUE(nearVec4(Vec4::zero(), Vec4(0.0f, 0.0f, 0.0f, 0.0f)));
    EXPECT_TRUE(nearVec4(Vec4::one(), Vec4(1.0f, 1.0f, 1.0f, 1.0f)));
    EXPECT_TRUE(nearVec4(Vec4::unitX(), Vec4(1.0f, 0.0f, 0.0f, 0.0f)));
    EXPECT_TRUE(nearVec4(Vec4::unitY(), Vec4(0.0f, 1.0f, 0.0f, 0.0f)));
    EXPECT_TRUE(nearVec4(Vec4::unitZ(), Vec4(0.0f, 0.0f, 1.0f, 0.0f)));
    EXPECT_TRUE(nearVec4(Vec4::unitW(), Vec4(0.0f, 0.0f, 0.0f, 1.0f)));
}

TEST(Vec4Test, ComponentwiseMulDiv)
{
    const Vec4 A(2.0f, 4.0f, 6.0f, 8.0f);
    const Vec4 B(1.0f, 2.0f, 3.0f, 4.0f);
    EXPECT_TRUE(nearVec4(A * B, Vec4(2.0f, 8.0f, 18.0f, 32.0f)));
    EXPECT_TRUE(nearVec4(A / B, Vec4(2.0f, 2.0f, 2.0f, 2.0f)));
}

TEST(Vec4Test, UnaryNegate)
{
    EXPECT_TRUE(nearVec4(-Vec4(1.0f, -2.0f, 3.0f, -4.0f), Vec4(-1.0f, 2.0f, -3.0f, 4.0f)));
}

TEST(Vec4Test, CompoundAssignment)
{
    Vec4 V(1.0f, 2.0f, 3.0f, 4.0f);
    V += Vec4(10.0f, 10.0f, 10.0f, 10.0f);
    EXPECT_TRUE(nearVec4(V, Vec4(11.0f, 12.0f, 13.0f, 14.0f)));
    V -= Vec4(1.0f, 2.0f, 3.0f, 4.0f);
    EXPECT_TRUE(nearVec4(V, Vec4(10.0f, 10.0f, 10.0f, 10.0f)));
    V *= Vec4(2.0f, 3.0f, 4.0f, 5.0f);
    EXPECT_TRUE(nearVec4(V, Vec4(20.0f, 30.0f, 40.0f, 50.0f)));
    V *= 0.5f;
    EXPECT_TRUE(nearVec4(V, Vec4(10.0f, 15.0f, 20.0f, 25.0f)));
}

TEST(Vec4Test, Equality)
{
    const Vec4 A(1.0f, 2.0f, 3.0f, 4.0f);
    EXPECT_EQ(A, Vec4(1.0f, 2.0f, 3.0f, 4.0f));
    EXPECT_NE(A, Vec4(1.0f, 2.0f, 3.0f, 4.5f));
    EXPECT_NE(A, Vec4(1.0f, 2.0f, 3.5f, 4.0f));
}

TEST(Vec4Test, MinMaxAbs)
{
    const Vec4 A(1.0f, -2.0f, 3.0f, -4.0f);
    const Vec4 B(-1.0f, 2.0f, -3.0f, 4.0f);
    EXPECT_TRUE(nearVec4(min(A, B), Vec4(-1.0f, -2.0f, -3.0f, -4.0f)));
    EXPECT_TRUE(nearVec4(max(A, B), Vec4(1.0f, 2.0f, 3.0f, 4.0f)));
    EXPECT_TRUE(nearVec4(abs(A), Vec4(1.0f, 2.0f, 3.0f, 4.0f)));
}

TEST(Vec3Test, StaticFactories)
{
    EXPECT_TRUE(nearVec3(Vec3::zero(), Vec3(0.0f, 0.0f, 0.0f)));
    EXPECT_TRUE(nearVec3(Vec3::one(), Vec3(1.0f, 1.0f, 1.0f)));
    EXPECT_TRUE(nearVec3(Vec3::unitX(), Vec3(1.0f, 0.0f, 0.0f)));
    EXPECT_TRUE(nearVec3(Vec3::unitY(), Vec3(0.0f, 1.0f, 0.0f)));
    EXPECT_TRUE(nearVec3(Vec3::unitZ(), Vec3(0.0f, 0.0f, 1.0f)));
}

TEST(Vec3Test, ComponentwiseMul)
{
    EXPECT_TRUE(nearVec3(Vec3(2.0f, 3.0f, 4.0f) * Vec3(5.0f, 6.0f, 7.0f), Vec3(10.0f, 18.0f, 28.0f)));
}

TEST(Vec3Test, MinMaxLerp)
{
    const Vec3 A(1.0f, -2.0f, 3.0f);
    const Vec3 B(-1.0f, 2.0f, 0.0f);
    EXPECT_TRUE(nearVec3(min(A, B), Vec3(-1.0f, -2.0f, 0.0f)));
    EXPECT_TRUE(nearVec3(max(A, B), Vec3(1.0f, 2.0f, 3.0f)));
    EXPECT_TRUE(nearVec3(lerp(A, B, 0.5f), Vec3(0.0f, 0.0f, 1.5f)));
}

TEST(Vec3Test, CompoundAssignment)
{
    Vec3 V(1.0f, 2.0f, 3.0f);
    V += Vec3(10.0f, 10.0f, 10.0f);
    EXPECT_TRUE(nearVec3(V, Vec3(11.0f, 12.0f, 13.0f)));
    V -= Vec3(1.0f, 2.0f, 3.0f);
    EXPECT_TRUE(nearVec3(V, Vec3(10.0f, 10.0f, 10.0f)));
    V *= 0.5f;
    EXPECT_TRUE(nearVec3(V, Vec3(5.0f, 5.0f, 5.0f)));
}

TEST(Vec3Test, Equality)
{
    EXPECT_EQ(Vec3(1.0f, 2.0f, 3.0f), Vec3(1.0f, 2.0f, 3.0f));
    EXPECT_NE(Vec3(1.0f, 2.0f, 3.0f), Vec3(1.0f, 2.0f, 3.5f));
}

TEST(Vec2Test, AddSubNegate)
{
    const Vec2 A(1.0f, 2.0f);
    const Vec2 B(10.0f, 20.0f);
    EXPECT_EQ(A + B, Vec2(11.0f, 22.0f));
    EXPECT_EQ(B - A, Vec2(9.0f, 18.0f));
    EXPECT_EQ(-A, Vec2(-1.0f, -2.0f));
}

TEST(Vec2Test, ScalarMulAndEquality)
{
    const Vec2 A(3.0f, 4.0f);
    EXPECT_EQ(A * 2.0f, Vec2(6.0f, 8.0f));
    EXPECT_EQ(2.0f * A, Vec2(6.0f, 8.0f));
    EXPECT_NE(A, Vec2(3.0f, 5.0f));
}

TEST(Vec2Test, StaticFactories)
{
    EXPECT_EQ(Vec2::zero(), Vec2(0.0f, 0.0f));
    EXPECT_EQ(Vec2::one(), Vec2(1.0f, 1.0f));
    EXPECT_EQ(Vec2::unitX(), Vec2(1.0f, 0.0f));
    EXPECT_EQ(Vec2::unitY(), Vec2(0.0f, 1.0f));
}
