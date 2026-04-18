#pragma once

/// @file
/// @brief Row-major Mat3 / Mat4 with row-vector transform convention (V' = V * M).

#include "CoreExport.h"
#include "Math/SIMD/SimdOps.h"
#include "Math/Vector.h"

namespace goleta::math
{

/// @brief 4x4 float matrix stored row-major. @note Convention is row-vector on the
///        left (`V' = V * M`); compose transforms left-to-right in application order.
struct GOLETA_ALIGN16 Mat4
{
    simd::Vec4f Rows[4];

    /// @brief Leaves rows uninitialized.
    Mat4() = default;
    /// @brief Constructs from four explicit row vectors.
    Mat4(Vec4 R0, Vec4 R1, Vec4 R2, Vec4 R3);

    /// @brief Returns the 4x4 identity matrix.
    static Mat4 identity();
    /// @brief Returns an all-zero matrix.
    static Mat4 zero();

    /// @brief Builds a pure translation matrix. Row 3 holds @p T.
    static Mat4 translation(Vec3 T);
    /// @brief Builds a non-uniform scale matrix with per-axis factors @p S.
    static Mat4 scale(Vec3 S);
    /// @brief Builds a uniform scale matrix with factor @p S on all axes.
    static Mat4 scale(float S);
    /// @brief Builds a right-handed rotation around the X axis.
    /// @param Radians Angle in radians; positive rotates +Y toward +Z.
    static Mat4 rotationX(float Radians);
    /// @brief Builds a right-handed rotation around the Y axis.
    /// @param Radians Angle in radians; positive rotates +Z toward +X.
    static Mat4 rotationY(float Radians);
    /// @brief Builds a right-handed rotation around the Z axis.
    /// @param Radians Angle in radians; positive rotates +X toward +Y.
    static Mat4 rotationZ(float Radians);
    /// @brief Builds a right-handed rotation around an arbitrary axis (Rodrigues).
    /// @param Axis Rotation axis; normalized internally.
    /// @param Radians Angle in radians.
    static CORE_API Mat4 rotationAxis(Vec3 Axis, float Radians);

    /// @brief Left-handed perspective projection mapping clip-space Z to [0, 1].
    /// @param FovY Vertical field of view in radians.
    static CORE_API Mat4 perspectiveFovLH(float FovY, float AspectRatio, float NearZ, float FarZ);
    /// @brief Right-handed perspective projection mapping clip-space Z to [0, 1].
    /// @param FovY Vertical field of view in radians.
    static CORE_API Mat4 perspectiveFovRH(float FovY, float AspectRatio, float NearZ, float FarZ);
    /// @brief Left-handed orthographic projection mapping clip-space Z to [0, 1].
    static CORE_API Mat4 orthographicLH(float Width, float Height, float NearZ, float FarZ);
    /// @brief Right-handed orthographic projection mapping clip-space Z to [0, 1].
    static CORE_API Mat4 orthographicRH(float Width, float Height, float NearZ, float FarZ);
    /// @brief Left-handed view matrix looking from @p Eye toward @p Target.
    /// @param Up World-space up vector used to build the basis; need not be orthogonal to the forward vector.
    static CORE_API Mat4 lookAtLH(Vec3 Eye, Vec3 Target, Vec3 Up);
    /// @brief Right-handed view matrix looking from @p Eye toward @p Target.
    /// @param Up World-space up vector used to build the basis.
    static CORE_API Mat4 lookAtRH(Vec3 Eye, Vec3 Target, Vec3 Up);

    /// @brief Returns the @p I-th row (0-based).
    Vec4 row(int I) const;
    /// @brief Returns the @p I-th column (0-based).
    Vec4 column(int I) const;
};

/// @brief Elementwise matrix addition.
Mat4 operator+(const Mat4& A, const Mat4& B);
/// @brief Elementwise matrix subtraction.
Mat4 operator-(const Mat4& A, const Mat4& B);
/// @brief Matrix product in row-vector convention: `(A * B).row[i] = A.row[i] * B`.
Mat4 operator*(const Mat4& A, const Mat4& B);
/// @brief Column-vector form `M * V`. Slower than `V * M`; prefer the row-vector form where possible.
Vec4 operator*(const Mat4& M, Vec4 V);
/// @brief Row-vector form: `V * M = V.x*Row0 + V.y*Row1 + V.z*Row2 + V.w*Row3`.
Vec4 operator*(Vec4 V, const Mat4& M);

/// @brief Returns the transpose of @p M.
Mat4 transpose(const Mat4& M);
/// @brief Returns the full 4x4 inverse.
/// @note Uses cofactor expansion; undefined if @p M is singular (det ~ 0).
CORE_API Mat4 inverse(const Mat4& M);
/// @brief Returns the determinant of @p M.
CORE_API float determinant(const Mat4& M);

/// @brief Transforms @p P as a point (implicit W = 1), applying full affine transform.
Vec3 transformPoint(const Mat4& M, Vec3 P);
/// @brief Transforms @p D as a direction (implicit W = 0), ignoring translation.
/// @note Assumes the upper-left 3x3 is linear; projection matrices do not qualify.
Vec3 transformDirection(const Mat4& M, Vec3 D);

/// @brief 3x3 matrix. Each row is padded to four SIMD lanes; lane W unused.
struct GOLETA_ALIGN16 Mat3
{
    simd::Vec4f Rows[3];

    /// @brief Leaves rows uninitialized.
    Mat3() = default;
    /// @brief Constructs from three explicit row vectors.
    Mat3(Vec3 R0, Vec3 R1, Vec3 R2);

    /// @brief Returns the 3x3 identity matrix.
    static Mat3 identity();
    /// @brief Returns an all-zero matrix.
    static Mat3 zero();

    /// @brief Builds a non-uniform scale matrix with per-axis factors @p S.
    static Mat3 scale(Vec3 S);
    /// @brief Builds a right-handed rotation around the X axis.
    static Mat3 rotationX(float Radians);
    /// @brief Builds a right-handed rotation around the Y axis.
    static Mat3 rotationY(float Radians);
    /// @brief Builds a right-handed rotation around the Z axis.
    static Mat3 rotationZ(float Radians);
    /// @brief Builds a right-handed rotation around an arbitrary axis (Rodrigues).
    /// @param Axis Rotation axis; normalized internally.
    static CORE_API Mat3 rotationAxis(Vec3 Axis, float Radians);

    /// @brief Returns the @p I-th row (0-based).
    Vec3 row(int I) const;
    /// @brief Returns the @p I-th column (0-based).
    Vec3 column(int I) const;
};

/// @brief Elementwise matrix addition.
Mat3 operator+(const Mat3& A, const Mat3& B);
/// @brief Elementwise matrix subtraction.
Mat3 operator-(const Mat3& A, const Mat3& B);
/// @brief Matrix product in row-vector convention: `(A * B).row[i] = A.row[i] * B`.
Mat3 operator*(const Mat3& A, const Mat3& B);
/// @brief Column-vector form `M * V` (equivalent to `V * transpose(M)`).
Vec3 operator*(const Mat3& M, Vec3 V);
/// @brief Row-vector form: `V * M = V.x*Row0 + V.y*Row1 + V.z*Row2`.
Vec3 operator*(Vec3 V, const Mat3& M);

/// @brief Returns the transpose of @p M.
Mat3 transpose(const Mat3& M);
/// @brief Returns the 3x3 inverse via the adjugate.
/// @note Undefined if @p M is singular (det ~ 0).
CORE_API Mat3 inverse(const Mat3& M);
/// @brief Returns the scalar triple product (row 0) · (row 1 x row 2).
CORE_API float determinant(const Mat3& M);

} // namespace goleta::math

#include "Math/Matrix.inl"
