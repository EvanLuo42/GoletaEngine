/// @file
/// @brief Unit tests for Mat3 / Mat4.

#include <numbers>

#include "MathTestHelpers.h"

using namespace goleta::math;
using goleta::math::testing::nearFloat;
using goleta::math::testing::nearMat4;
using goleta::math::testing::nearVec3;
using goleta::math::testing::nearVec4;

TEST(Mat4Test, IdentityTimesIdentityIsIdentity)
{
    const Mat4 I = Mat4::identity();
    EXPECT_TRUE(nearMat4(I * I, I));
}

TEST(Mat4Test, TransposeIsInvolution)
{
    const Mat4 M = Mat4::translation(Vec3(1.0f, 2.0f, 3.0f)) * Mat4::rotationY(0.7f);
    EXPECT_TRUE(nearMat4(transpose(transpose(M)), M));
}

TEST(Mat4Test, RowVectorTransformFollowsConvention)
{
    const Mat4 T = Mat4::translation(Vec3(10.0f, 20.0f, 30.0f));
    const Vec4 P(1.0f, 2.0f, 3.0f, 1.0f);
    EXPECT_TRUE(nearVec4(P * T, Vec4(11.0f, 22.0f, 33.0f, 1.0f)));
}

TEST(Mat4Test, RotationYMovesXTowardNegZ)
{
    const Mat4 R = Mat4::rotationY(std::numbers::pi_v<float> * 0.5f);
    EXPECT_TRUE(nearVec3(transformDirection(R, Vec3::unitX()), Vec3(0.0f, 0.0f, -1.0f), 1e-5f));
}

TEST(Mat4Test, InverseRoundtripToIdentity)
{
    const Mat4 M =
        Mat4::translation(Vec3(1.0f, 2.0f, 3.0f)) * Mat4::rotationZ(0.6f) * Mat4::scale(Vec3(2.0f, 0.5f, 3.0f));
    EXPECT_TRUE(nearMat4(M * inverse(M), Mat4::identity(), 1e-4f));
    EXPECT_TRUE(nearMat4(inverse(M) * M, Mat4::identity(), 1e-4f));
}

TEST(Mat4Test, DeterminantOfIdentityIsOne) { EXPECT_FLOAT_EQ(determinant(Mat4::identity()), 1.0f); }

TEST(Mat4Test, DeterminantOfScale) { EXPECT_TRUE(nearFloat(determinant(Mat4::scale(Vec3(2.0f, 3.0f, 4.0f))), 24.0f)); }

TEST(Mat4Test, RotationAxisMatchesRotationY)
{
    const Mat4 A = Mat4::rotationY(0.4f);
    const Mat4 B = Mat4::rotationAxis(Vec3::unitY(), 0.4f);
    EXPECT_TRUE(nearMat4(A, B, 1e-5f));
}

TEST(Mat4Test, LookAtLHPlacesTargetOnPositiveZ)
{
    const Vec3 Eye(0.0f, 0.0f, -5.0f);
    const Vec3 Target(0.0f, 0.0f, 0.0f);
    const Mat4 View = Mat4::lookAtLH(Eye, Target, Vec3::unitY());
    const Vec3 Seen = transformPoint(View, Target);
    EXPECT_TRUE(nearFloat(Seen.x(), 0.0f));
    EXPECT_TRUE(nearFloat(Seen.y(), 0.0f));
    EXPECT_GT(Seen.z(), 0.0f);
}

TEST(Mat4Test, PerspectiveFovLHMapsNearToZeroFarToOne)
{
    const Mat4 P = Mat4::perspectiveFovLH(1.0f, 1.0f, 1.0f, 100.0f);
    const Vec4 OnNear(0.0f, 0.0f, 1.0f, 1.0f);
    const Vec4 OnFar(0.0f, 0.0f, 100.0f, 1.0f);
    const Vec4 Near = OnNear * P;
    const Vec4 Far = OnFar * P;
    EXPECT_TRUE(nearFloat(Near.z() / Near.w(), 0.0f, 1e-4f));
    EXPECT_TRUE(nearFloat(Far.z() / Far.w(), 1.0f, 1e-4f));
}

TEST(Mat3Test, InverseRoundtrip)
{
    const Mat3 M = Mat3::rotationZ(0.5f) * Mat3::scale(Vec3(2.0f, 3.0f, 1.5f));
    const Mat3 Prod = M * inverse(M);
    for (int I = 0; I < 3; ++I)
    {
        EXPECT_TRUE(nearVec3(Prod.row(I), Mat3::identity().row(I), 1e-4f));
    }
}

TEST(Mat3Test, DeterminantOfRotationIsOne) { EXPECT_TRUE(nearFloat(determinant(Mat3::rotationZ(0.7f)), 1.0f)); }

TEST(Mat4Test, AdditionAndSubtraction)
{
    const Mat4 I = Mat4::identity();
    const Mat4 Sum = I + I;
    EXPECT_TRUE(nearVec4(Sum.row(0), Vec4(2.0f, 0.0f, 0.0f, 0.0f)));
    EXPECT_TRUE(nearVec4(Sum.row(3), Vec4(0.0f, 0.0f, 0.0f, 2.0f)));
    EXPECT_TRUE(nearMat4(Sum - I, I));
}

TEST(Mat4Test, RotationXMovesYTowardPosZ)
{
    const Mat4 R = Mat4::rotationX(std::numbers::pi_v<float> * 0.5f);
    EXPECT_TRUE(nearVec3(transformDirection(R, Vec3::unitY()), Vec3(0.0f, 0.0f, 1.0f), 1e-5f));
}

TEST(Mat4Test, RotationZMovesXTowardPosY)
{
    const Mat4 R = Mat4::rotationZ(std::numbers::pi_v<float> * 0.5f);
    EXPECT_TRUE(nearVec3(transformDirection(R, Vec3::unitX()), Vec3(0.0f, 1.0f, 0.0f), 1e-5f));
}

TEST(Mat4Test, RotationAxisArbitraryVectorPreservesLength)
{
    const Vec3 Axis = normalize(Vec3(1.0f, 1.0f, 0.0f));
    const Mat4 R = Mat4::rotationAxis(Axis, 0.9f);
    const Vec3 V(0.3f, -0.7f, 1.2f);
    EXPECT_TRUE(nearFloat(length(transformDirection(R, V)), length(V), 1e-5f));
}

TEST(Mat4Test, ScaleFloatEqualsScaleVec3)
{
    EXPECT_TRUE(nearMat4(Mat4::scale(3.0f), Mat4::scale(Vec3(3.0f, 3.0f, 3.0f))));
}

TEST(Mat4Test, PerspectiveFovRHMapsNearAndFar)
{
    const Mat4 P = Mat4::perspectiveFovRH(1.0f, 1.0f, 1.0f, 100.0f);
    const Vec4 OnNear(0.0f, 0.0f, -1.0f, 1.0f);
    const Vec4 OnFar(0.0f, 0.0f, -100.0f, 1.0f);
    const Vec4 Near = OnNear * P;
    const Vec4 Far = OnFar * P;
    EXPECT_TRUE(nearFloat(Near.z() / Near.w(), 0.0f, 1e-4f));
    EXPECT_TRUE(nearFloat(Far.z() / Far.w(), 1.0f, 1e-4f));
}

TEST(Mat4Test, OrthographicLHMapsNearToZeroFarToOne)
{
    const Mat4 O = Mat4::orthographicLH(10.0f, 10.0f, 1.0f, 100.0f);
    EXPECT_TRUE(nearFloat((Vec4(0.0f, 0.0f, 1.0f, 1.0f) * O).z(), 0.0f, 1e-4f));
    EXPECT_TRUE(nearFloat((Vec4(0.0f, 0.0f, 100.0f, 1.0f) * O).z(), 1.0f, 1e-4f));
}

TEST(Mat4Test, OrthographicRHMapsNearToZeroFarToOne)
{
    const Mat4 O = Mat4::orthographicRH(10.0f, 10.0f, 1.0f, 100.0f);
    EXPECT_TRUE(nearFloat((Vec4(0.0f, 0.0f, -1.0f, 1.0f) * O).z(), 0.0f, 1e-4f));
    EXPECT_TRUE(nearFloat((Vec4(0.0f, 0.0f, -100.0f, 1.0f) * O).z(), 1.0f, 1e-4f));
}

TEST(Mat4Test, LookAtRHPlacesTargetOnNegativeZ)
{
    const Vec3 Eye(0.0f, 0.0f, 5.0f);
    const Vec3 Target(0.0f, 0.0f, 0.0f);
    const Mat4 View = Mat4::lookAtRH(Eye, Target, Vec3::unitY());
    const Vec3 Seen = transformPoint(View, Target);
    EXPECT_TRUE(nearFloat(Seen.x(), 0.0f));
    EXPECT_TRUE(nearFloat(Seen.y(), 0.0f));
    EXPECT_LT(Seen.z(), 0.0f);
}

TEST(Mat4Test, ColumnExtractsColumnAcrossRows)
{
    const Mat4 M(Vec4(1.0f, 2.0f, 3.0f, 4.0f),
                 Vec4(5.0f, 6.0f, 7.0f, 8.0f),
                 Vec4(9.0f, 10.0f, 11.0f, 12.0f),
                 Vec4(13.0f, 14.0f, 15.0f, 16.0f));
    EXPECT_TRUE(nearVec4(M.column(0), Vec4(1.0f, 5.0f, 9.0f, 13.0f)));
    EXPECT_TRUE(nearVec4(M.column(3), Vec4(4.0f, 8.0f, 12.0f, 16.0f)));
}

TEST(Mat4Test, RowVectorMultiplyAgreesWithComposedRotation)
{
    const Mat4 A = Mat4::rotationZ(0.3f);
    const Mat4 B = Mat4::rotationY(0.4f);
    const Vec4 V(1.0f, 0.5f, -0.5f, 0.0f);
    EXPECT_TRUE(nearVec4(V * (A * B), (V * A) * B, 1e-5f));
}

TEST(Mat4Test, MatVecColumnFormEqualsVecMatTransposed)
{
    const Mat4 M = Mat4::rotationZ(0.4f) * Mat4::translation(Vec3(1.0f, 2.0f, 3.0f));
    const Vec4 V(5.0f, 6.0f, 7.0f, 1.0f);
    EXPECT_TRUE(nearVec4(M * V, V * transpose(M), 1e-4f));
}

TEST(Mat4Test, TransformDirectionIgnoresTranslation)
{
    const Mat4 M = Mat4::translation(Vec3(100.0f, 200.0f, 300.0f)) * Mat4::rotationY(0.7f);
    const Vec3 D(1.0f, 0.0f, 0.0f);
    const Vec3 Rotated = transformDirection(M, D);
    EXPECT_TRUE(nearFloat(length(Rotated), 1.0f, 1e-5f));
}

TEST(Mat3Test, AdditionAndSubtraction)
{
    const Mat3 I = Mat3::identity();
    const Mat3 Sum = I + I;
    EXPECT_TRUE(nearVec3(Sum.row(0), Vec3(2.0f, 0.0f, 0.0f)));
    const Mat3 Back = Sum - I;
    EXPECT_TRUE(nearVec3(Back.row(0), Vec3(1.0f, 0.0f, 0.0f)));
    EXPECT_TRUE(nearVec3(Back.row(2), Vec3(0.0f, 0.0f, 1.0f)));
}

TEST(Mat3Test, RotationXRotatesYToZ)
{
    const Mat3 R = Mat3::rotationX(std::numbers::pi_v<float> * 0.5f);
    EXPECT_TRUE(nearVec3(Vec3::unitY() * R, Vec3(0.0f, 0.0f, 1.0f), 1e-5f));
}

TEST(Mat3Test, RotationYRotatesZToX)
{
    const Mat3 R = Mat3::rotationY(std::numbers::pi_v<float> * 0.5f);
    EXPECT_TRUE(nearVec3(Vec3::unitZ() * R, Vec3(1.0f, 0.0f, 0.0f), 1e-5f));
}

TEST(Mat3Test, RotationAxisMatchesRotationZ)
{
    EXPECT_TRUE(nearVec3(
        Vec3::unitX() * Mat3::rotationAxis(Vec3::unitZ(), 0.6f),
        Vec3::unitX() * Mat3::rotationZ(0.6f),
        1e-5f));
}

TEST(Mat3Test, MatVecColumnForm)
{
    const Mat3 M = Mat3::scale(Vec3(2.0f, 3.0f, 4.0f));
    EXPECT_TRUE(nearVec3(M * Vec3(1.0f, 1.0f, 1.0f), Vec3(2.0f, 3.0f, 4.0f)));
}

TEST(Mat3Test, TransposeTwiceIsIdentity)
{
    const Mat3 M = Mat3::rotationZ(0.3f) * Mat3::scale(Vec3(1.5f, 2.0f, 0.5f));
    const Mat3 TT = transpose(transpose(M));
    for (int I = 0; I < 3; ++I)
    {
        EXPECT_TRUE(nearVec3(TT.row(I), M.row(I), 1e-5f));
    }
}

TEST(Mat3Test, ColumnExtractsColumnAcrossRows)
{
    const Mat3 M(Vec3(1.0f, 2.0f, 3.0f), Vec3(4.0f, 5.0f, 6.0f), Vec3(7.0f, 8.0f, 9.0f));
    EXPECT_TRUE(nearVec3(M.column(0), Vec3(1.0f, 4.0f, 7.0f)));
    EXPECT_TRUE(nearVec3(M.column(1), Vec3(2.0f, 5.0f, 8.0f)));
    EXPECT_TRUE(nearVec3(M.column(2), Vec3(3.0f, 6.0f, 9.0f)));
}
