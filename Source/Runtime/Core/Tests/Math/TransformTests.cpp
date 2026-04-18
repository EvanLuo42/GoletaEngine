/// @file
/// @brief Unit tests for Transform.

#include "MathTestHelpers.h"

using namespace goleta::math;
using goleta::math::testing::nearVec3;

TEST(TransformTest, IdentityLeavesPointUnchanged)
{
    const Transform T = Transform::identity();
    const Vec3 P(1.0f, 2.0f, 3.0f);
    EXPECT_TRUE(nearVec3(transformPoint(T, P), P));
}

TEST(TransformTest, TranslationAddsToPoint)
{
    const Transform T(Vec3(10.0f, 20.0f, 30.0f), Quat::identity(), Vec3::one());
    EXPECT_TRUE(nearVec3(transformPoint(T, Vec3(1.0f, 2.0f, 3.0f)), Vec3(11.0f, 22.0f, 33.0f)));
}

TEST(TransformTest, ScaleAppliesBeforeRotation)
{
    const Transform T(Vec3::zero(),
                      Quat::fromAxisAngle(Vec3::unitZ(), 3.14159265f * 0.5f),
                      Vec3(2.0f, 1.0f, 1.0f));
    EXPECT_TRUE(nearVec3(transformPoint(T, Vec3::unitX()), Vec3(0.0f, 2.0f, 0.0f), 1e-5f));
}

// inverse(Transform) is only exact when Scale is uniform; rotation does not
// commute with non-uniform scale, so the decomposed inverse cannot round-trip
// in that case. For non-uniform scale, go through toMatrix() + inverse(Mat4).
TEST(TransformTest, InverseRoundtripOnPointUniformScale)
{
    const Transform T(Vec3(1.0f, 2.0f, 3.0f),
                      Quat::fromAxisAngle(Vec3(1.0f, 1.0f, 0.0f), 0.7f),
                      Vec3(2.0f, 2.0f, 2.0f));
    const Transform Inv = inverse(T);
    const Vec3 P(5.0f, -2.0f, 1.0f);
    EXPECT_TRUE(nearVec3(transformPoint(Inv, transformPoint(T, P)), P, 1e-4f));
}

TEST(TransformTest, MatrixInverseHandlesNonUniformScale)
{
    const Transform T(Vec3(1.0f, 2.0f, 3.0f),
                      Quat::fromAxisAngle(Vec3(1.0f, 1.0f, 0.0f), 0.7f),
                      Vec3(2.0f, 3.0f, 0.5f));
    const Mat4 M = T.toMatrix();
    const Mat4 InvM = inverse(M);
    const Vec3 P(5.0f, -2.0f, 1.0f);
    EXPECT_TRUE(nearVec3(transformPoint(InvM, transformPoint(M, P)), P, 1e-4f));
}

TEST(TransformTest, ComposeIsAssociativeOnPoint)
{
    const Transform A(Vec3(1.0f, 0.0f, 0.0f), Quat::fromAxisAngle(Vec3::unitZ(), 0.3f), Vec3::one());
    const Transform B(Vec3(0.0f, 2.0f, 0.0f), Quat::fromAxisAngle(Vec3::unitY(), 0.4f), Vec3::one());
    const Transform C(Vec3(0.0f, 0.0f, 3.0f), Quat::fromAxisAngle(Vec3::unitX(), 0.5f), Vec3::one());
    const Vec3 P(1.0f, 1.0f, 1.0f);
    const Vec3 Left = transformPoint(A * B * C, P);
    const Vec3 Right = transformPoint(A, transformPoint(B, transformPoint(C, P)));
    EXPECT_TRUE(nearVec3(Left, Right, 1e-4f));
}

TEST(TransformTest, ToMatrixMatchesTransformPoint)
{
    const Transform T(Vec3(1.0f, 2.0f, 3.0f),
                      Quat::fromAxisAngle(Vec3::unitY(), 0.6f),
                      Vec3(2.0f, 2.0f, 2.0f));
    const Mat4 M = T.toMatrix();
    const Vec3 P(0.5f, -1.0f, 2.5f);
    EXPECT_TRUE(nearVec3(transformPoint(M, P), transformPoint(T, P), 1e-4f));
}

TEST(TransformTest, TransformDirectionIgnoresTranslation)
{
    const Transform T(Vec3(100.0f, -50.0f, 20.0f),
                      Quat::fromAxisAngle(Vec3::unitZ(), 1.2f),
                      Vec3(2.0f, 2.0f, 2.0f));
    const Vec3 D = Vec3::unitX();
    const Vec3 RotatedOnly = rotate(T.Rotation, D * T.Scale);
    EXPECT_TRUE(nearVec3(transformDirection(T, D), RotatedOnly, 1e-5f));
}

TEST(TransformTest, LerpMidpointBlendsTranslationAndScale)
{
    const Transform A(Vec3::zero(), Quat::identity(), Vec3(1.0f, 1.0f, 1.0f));
    const Transform B(Vec3(10.0f, 20.0f, 30.0f), Quat::identity(), Vec3(3.0f, 3.0f, 3.0f));
    const Transform Mid = lerp(A, B, 0.5f);
    EXPECT_TRUE(nearVec3(Mid.Translation, Vec3(5.0f, 10.0f, 15.0f), 1e-5f));
    EXPECT_TRUE(nearVec3(Mid.Scale, Vec3(2.0f, 2.0f, 2.0f), 1e-5f));
}

TEST(TransformTest, InverseOfIdentityIsIdentity)
{
    const Transform T = Transform::identity();
    const Transform Inv = inverse(T);
    EXPECT_TRUE(nearVec3(Inv.Translation, Vec3::zero(), 1e-6f));
    EXPECT_TRUE(nearVec3(Inv.Scale, Vec3::one(), 1e-6f));
}
