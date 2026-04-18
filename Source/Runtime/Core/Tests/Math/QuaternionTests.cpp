/// @file
/// @brief Unit tests for Quat.

#include <numbers>

#include "MathTestHelpers.h"

using namespace goleta::math;
using goleta::math::testing::nearFloat;
using goleta::math::testing::nearVec3;

TEST(QuatTest, IdentityRotatesNothing)
{
    const Quat Q = Quat::identity();
    const Vec3 V(1.0f, 2.0f, 3.0f);
    EXPECT_TRUE(nearVec3(rotate(Q, V), V));
}

TEST(QuatTest, AxisAngleNinetyAroundZRotatesXToY)
{
    const Quat Q = Quat::fromAxisAngle(Vec3::unitZ(), 3.14159265f * 0.5f);
    EXPECT_TRUE(nearVec3(rotate(Q, Vec3::unitX()), Vec3::unitY(), 1e-5f));
}

TEST(QuatTest, ConjugateInverts)
{
    const Quat Q = Quat::fromAxisAngle(Vec3(1.0f, 1.0f, 0.0f), 0.6f);
    const Vec3 V(0.3f, -0.7f, 1.2f);
    EXPECT_TRUE(nearVec3(rotate(conjugate(Q), rotate(Q, V)), V, 1e-5f));
}

TEST(QuatTest, NormalizePreservesRotation)
{
    const Quat Q = Quat::fromAxisAngle(Vec3::unitY(), 0.8f);
    const Quat Scaled = Q * 3.0f;
    const Quat Renormed = normalize(Scaled);
    const Vec3 V(1.0f, 0.0f, 0.0f);
    EXPECT_TRUE(nearVec3(rotate(Q, V), rotate(Renormed, V), 1e-5f));
}

TEST(QuatTest, SlerpEndpointsReturnOriginals)
{
    const Quat A = Quat::identity();
    const Quat B = Quat::fromAxisAngle(Vec3::unitY(), 1.0f);
    EXPECT_TRUE(nearFloat(std::fabs(dot(slerp(A, B, 0.0f), A)), 1.0f, 1e-4f));
    EXPECT_TRUE(nearFloat(std::fabs(dot(slerp(A, B, 1.0f), B)), 1.0f, 1e-4f));
}

TEST(QuatTest, SlerpMidpointRotatesHalfway)
{
    const Quat A = Quat::identity();
    const Quat B = Quat::fromAxisAngle(Vec3::unitZ(), 3.14159265f * 0.5f);
    const Quat Mid = slerp(A, B, 0.5f);
    const Vec3 V = rotate(Mid, Vec3::unitX());
    EXPECT_TRUE(nearFloat(V.x(), std::cos(3.14159265f * 0.25f), 1e-4f));
    EXPECT_TRUE(nearFloat(V.y(), std::sin(3.14159265f * 0.25f), 1e-4f));
    EXPECT_TRUE(nearFloat(V.z(), 0.0f, 1e-4f));
}

TEST(QuatTest, FromEulerAroundYOnlyAffectsYawed)
{
    const Quat Q = Quat::fromEuler(0.0f, 0.5f, 0.0f);
    EXPECT_TRUE(nearVec3(rotate(Q, Vec3::unitY()), Vec3::unitY(), 1e-5f));
}

TEST(QuatTest, LengthOfUnitQuatIsOne)
{
    const Quat Q = Quat::fromAxisAngle(Vec3::unitZ(), 0.8f);
    EXPECT_TRUE(nearFloat(length(Q), 1.0f, 1e-5f));
}

TEST(QuatTest, NormalizeRestoresUnitLength)
{
    const Quat Q = Quat::fromAxisAngle(Vec3::unitX(), 0.3f) * 4.2f;
    EXPECT_TRUE(nearFloat(length(normalize(Q)), 1.0f, 1e-5f));
}

TEST(QuatTest, AddSubNegate)
{
    const Quat A(1.0f, 2.0f, 3.0f, 4.0f);
    const Quat B(0.5f, 1.0f, 1.5f, 2.0f);
    const Quat Sum = A + B;
    EXPECT_FLOAT_EQ(Sum.x(), 1.5f);
    EXPECT_FLOAT_EQ(Sum.w(), 6.0f);
    const Quat Diff = A - B;
    EXPECT_FLOAT_EQ(Diff.x(), 0.5f);
    EXPECT_FLOAT_EQ(Diff.w(), 2.0f);
    const Quat Neg = -A;
    EXPECT_FLOAT_EQ(Neg.x(), -1.0f);
    EXPECT_FLOAT_EQ(Neg.w(), -4.0f);
}

TEST(QuatTest, Equality)
{
    const Quat A(0.1f, 0.2f, 0.3f, 0.4f);
    EXPECT_EQ(A, Quat(0.1f, 0.2f, 0.3f, 0.4f));
    EXPECT_NE(A, Quat(0.1f, 0.2f, 0.3f, 0.5f));
}

TEST(QuatTest, HamiltonProductComposesRotations)
{
    // A*B applied to V == A(B(V))
    const Quat A = Quat::fromAxisAngle(Vec3::unitZ(), std::numbers::pi_v<float> * 0.5f);
    const Quat B = Quat::fromAxisAngle(Vec3::unitX(), std::numbers::pi_v<float> * 0.5f);
    const Vec3 V = Vec3::unitY();
    EXPECT_TRUE(nearVec3(rotate(A * B, V), rotate(A, rotate(B, V)), 1e-5f));
}

TEST(QuatTest, FromEulerPitchAroundXRotatesYToZ)
{
    const Quat Q = Quat::fromEuler(std::numbers::pi_v<float> * 0.5f, 0.0f, 0.0f);
    EXPECT_TRUE(nearVec3(rotate(Q, Vec3::unitY()), Vec3::unitZ(), 1e-5f));
}

TEST(QuatTest, FromEulerRollAroundZRotatesXToY)
{
    const Quat Q = Quat::fromEuler(0.0f, 0.0f, std::numbers::pi_v<float> * 0.5f);
    EXPECT_TRUE(nearVec3(rotate(Q, Vec3::unitX()), Vec3::unitY(), 1e-5f));
}

TEST(QuatTest, FromRotationMatrixTraceBranch)
{
    // rotationZ(pi/2): trace = 0+0+1 = 1 > 0 so hits the trace branch.
    const Quat Q = Quat::fromRotationMatrix(Mat3::rotationZ(std::numbers::pi_v<float> * 0.5f));
    EXPECT_TRUE(nearVec3(rotate(Q, Vec3::unitX()), Vec3::unitY(), 1e-5f));
}

TEST(QuatTest, FromRotationMatrixXDiagonalBranch)
{
    // rotationX(pi): m[0]=1 is largest diagonal entry.
    const Quat Q = Quat::fromRotationMatrix(Mat3::rotationX(std::numbers::pi_v<float>));
    EXPECT_TRUE(nearVec3(rotate(Q, Vec3::unitY()), -Vec3::unitY(), 1e-5f));
}

TEST(QuatTest, FromRotationMatrixYDiagonalBranch)
{
    // rotationY(pi): m[5]=1 is largest diagonal entry.
    const Quat Q = Quat::fromRotationMatrix(Mat3::rotationY(std::numbers::pi_v<float>));
    EXPECT_TRUE(nearVec3(rotate(Q, Vec3::unitX()), -Vec3::unitX(), 1e-5f));
}

TEST(QuatTest, FromRotationMatrixZDiagonalBranch)
{
    // rotationZ(pi): m[10]=1 is largest diagonal entry.
    const Quat Q = Quat::fromRotationMatrix(Mat3::rotationZ(std::numbers::pi_v<float>));
    EXPECT_TRUE(nearVec3(rotate(Q, Vec3::unitX()), -Vec3::unitX(), 1e-5f));
}

TEST(QuatTest, NlerpEndpointsReturnNormalizedOriginals)
{
    const Quat A = Quat::identity();
    const Quat B = Quat::fromAxisAngle(Vec3::unitY(), 0.7f);
    EXPECT_TRUE(nearFloat(std::fabs(dot(nlerp(A, B, 0.0f), A)), 1.0f, 1e-5f));
    EXPECT_TRUE(nearFloat(std::fabs(dot(nlerp(A, B, 1.0f), B)), 1.0f, 1e-5f));
}

TEST(QuatTest, InverseRotatesBack)
{
    const Quat Q = Quat::fromAxisAngle(Vec3(0.5f, 0.8f, 0.2f), 1.1f);
    const Vec3 V(0.4f, 1.1f, -0.6f);
    EXPECT_TRUE(nearVec3(rotate(inverse(Q), rotate(Q, V)), V, 1e-5f));
}

TEST(QuatTest, SlerpFlipsNegativeDotForShortArc)
{
    // dot(A, B) < 0 means B points to the opposite hemisphere; slerp should
    // flip its sign so interpolation follows the short arc (<180 degrees).
    const Quat A = Quat::identity();
    const Quat B = Quat::fromAxisAngle(Vec3::unitY(), 1.5f * std::numbers::pi_v<float>);
    const Quat Mid = slerp(A, B, 1.0f);
    const Vec3 V = Vec3::unitX();
    EXPECT_TRUE(nearVec3(rotate(Mid, V), rotate(B, V), 1e-4f));
}

TEST(QuatTest, SlerpSmallAngleFallsBackToNlerp)
{
    // dot > 0.9995 triggers the nlerp fallback to avoid dividing by sin(~0).
    const Quat A = Quat::identity();
    const Quat B = Quat::fromAxisAngle(Vec3::unitZ(), 0.001f);
    const Quat Mid = slerp(A, B, 0.5f);
    const Vec3 Rotated = rotate(Mid, Vec3::unitX());
    EXPECT_TRUE(nearFloat(Rotated.x(), std::cos(0.0005f), 1e-5f));
    EXPECT_TRUE(nearFloat(Rotated.y(), std::sin(0.0005f), 1e-5f));
}

TEST(QuatTest, NlerpFlipsNegativeDot)
{
    // dot < 0 inside nlerp's ternary: B is negated before the lerp so the
    // result represents the short-arc interpolation.
    const Quat A = Quat::fromAxisAngle(Vec3::unitX(), 0.4f);
    const Quat B = -A; // same rotation, opposite hemisphere, dot(A, B) = -1
    const Quat Mid = nlerp(A, B, 0.5f);
    const Vec3 V = Vec3::unitY();
    EXPECT_TRUE(nearVec3(rotate(Mid, V), rotate(A, V), 1e-4f));
}
