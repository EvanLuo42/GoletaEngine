/// @file
/// @brief Out-of-line matrix routines: inverse, determinant, projection,
///        lookAt, Rodrigues rotation.

#include "Math/Matrix.h"

#include <cmath>

namespace goleta::math
{

Mat4 Mat4::rotationAxis(const Vec3 Axis, const float Radians)
{
    const Vec3 N = normalize(Axis);
    const float Nx = N.x(), Ny = N.y(), Nz = N.z();
    const float C = std::cos(Radians);
    const float S = std::sin(Radians);
    const float T = 1.0f - C;
    return {Vec4(T * Nx * Nx + C, T * Nx * Ny + S * Nz, T * Nx * Nz - S * Ny, 0.0f),
            Vec4(T * Nx * Ny - S * Nz, T * Ny * Ny + C, T * Ny * Nz + S * Nx, 0.0f),
            Vec4(T * Nx * Nz + S * Ny, T * Ny * Nz - S * Nx, T * Nz * Nz + C, 0.0f), Vec4(0.0f, 0.0f, 0.0f, 1.0f)};
}

// LH: view-space looks down +Z. Clip-space Z mapped to [0, 1].
Mat4 Mat4::perspectiveFovLH(const float FovY, const float AspectRatio, const float NearZ, const float FarZ)
{
    const float H = 1.0f / std::tan(FovY * 0.5f);
    const float W = H / AspectRatio;
    const float A = FarZ / (FarZ - NearZ);
    const float B = -NearZ * A;
    return {Vec4(W, 0.0f, 0.0f, 0.0f), Vec4(0.0f, H, 0.0f, 0.0f), Vec4(0.0f, 0.0f, A, 1.0f), Vec4(0.0f, 0.0f, B, 0.0f)};
}

Mat4 Mat4::perspectiveFovRH(const float FovY, const float AspectRatio, const float NearZ, const float FarZ)
{
    const float H = 1.0f / std::tan(FovY * 0.5f);
    const float W = H / AspectRatio;
    const float A = FarZ / (NearZ - FarZ);
    const float B = NearZ * A;
    return {Vec4(W, 0.0f, 0.0f, 0.0f), Vec4(0.0f, H, 0.0f, 0.0f), Vec4(0.0f, 0.0f, A, -1.0f),
            Vec4(0.0f, 0.0f, B, 0.0f)};
}

Mat4 Mat4::orthographicLH(const float Width, const float Height, const float NearZ, const float FarZ)
{
    const float A = 1.0f / (FarZ - NearZ);
    return {Vec4(2.0f / Width, 0.0f, 0.0f, 0.0f), Vec4(0.0f, 2.0f / Height, 0.0f, 0.0f), Vec4(0.0f, 0.0f, A, 0.0f),
            Vec4(0.0f, 0.0f, -NearZ * A, 1.0f)};
}

Mat4 Mat4::orthographicRH(const float Width, const float Height, const float NearZ, float const FarZ)
{
    const float A = 1.0f / (NearZ - FarZ);
    return {Vec4(2.0f / Width, 0.0f, 0.0f, 0.0f), Vec4(0.0f, 2.0f / Height, 0.0f, 0.0f), Vec4(0.0f, 0.0f, A, 0.0f),
            Vec4(0.0f, 0.0f, NearZ * A, 1.0f)};
}

Mat4 Mat4::lookAtLH(const Vec3 Eye, const Vec3 Target, const Vec3 Up)
{
    const Vec3 Z = normalize(Target - Eye);
    const Vec3 X = normalize(cross(Up, Z));
    const Vec3 Y = cross(Z, X);
    return {Vec4(X.x(), Y.x(), Z.x(), 0.0f), Vec4(X.y(), Y.y(), Z.y(), 0.0f), Vec4(X.z(), Y.z(), Z.z(), 0.0f),
            Vec4(-dot(X, Eye), -dot(Y, Eye), -dot(Z, Eye), 1.0f)};
}
Mat4 Mat4::lookAtRH(const Vec3 Eye, const Vec3 Target, const Vec3 Up)
{
    const Vec3 Z = normalize(Eye - Target);
    const Vec3 X = normalize(cross(Up, Z));
    const Vec3 Y = cross(Z, X);
    return {Vec4(X.x(), Y.x(), Z.x(), 0.0f), Vec4(X.y(), Y.y(), Z.y(), 0.0f), Vec4(X.z(), Y.z(), Z.z(), 0.0f),
            Vec4(-dot(X, Eye), -dot(Y, Eye), -dot(Z, Eye), 1.0f)};
}

float determinant(const Mat4& M)
{
    alignas(16) float m[16];
    simd::store(m + 0, M.Rows[0]);
    simd::store(m + 4, M.Rows[1]);
    simd::store(m + 8, M.Rows[2]);
    simd::store(m + 12, M.Rows[3]);

    const float A = m[0] * (m[5] * (m[10] * m[15] - m[11] * m[14]) - m[6] * (m[9] * m[15] - m[11] * m[13]) +
                            m[7] * (m[9] * m[14] - m[10] * m[13]));
    const float B = m[1] * (m[4] * (m[10] * m[15] - m[11] * m[14]) - m[6] * (m[8] * m[15] - m[11] * m[12]) +
                            m[7] * (m[8] * m[14] - m[10] * m[12]));
    const float C = m[2] * (m[4] * (m[9] * m[15] - m[11] * m[13]) - m[5] * (m[8] * m[15] - m[11] * m[12]) +
                            m[7] * (m[8] * m[13] - m[9] * m[12]));
    const float D = m[3] * (m[4] * (m[9] * m[14] - m[10] * m[13]) - m[5] * (m[8] * m[14] - m[10] * m[12]) +
                            m[6] * (m[8] * m[13] - m[9] * m[12]));
    return A - B + C - D;
}

// Cofactor expansion. Correctness-focused; hand-vectorized inverse is a
// worthwhile upgrade when profiling proves this is hot.
Mat4 inverse(const Mat4& M)
{
    alignas(16) float m[16];
    simd::store(m + 0, M.Rows[0]);
    simd::store(m + 4, M.Rows[1]);
    simd::store(m + 8, M.Rows[2]);
    simd::store(m + 12, M.Rows[3]);

    float Inv[16];
    Inv[0] = m[5] * m[10] * m[15] - m[5] * m[11] * m[14] - m[9] * m[6] * m[15] + m[9] * m[7] * m[14] +
             m[13] * m[6] * m[11] - m[13] * m[7] * m[10];
    Inv[1] = -m[1] * m[10] * m[15] + m[1] * m[11] * m[14] + m[9] * m[2] * m[15] - m[9] * m[3] * m[14] -
             m[13] * m[2] * m[11] + m[13] * m[3] * m[10];
    Inv[2] = m[1] * m[6] * m[15] - m[1] * m[7] * m[14] - m[5] * m[2] * m[15] + m[5] * m[3] * m[14] +
             m[13] * m[2] * m[7] - m[13] * m[3] * m[6];
    Inv[3] = -m[1] * m[6] * m[11] + m[1] * m[7] * m[10] + m[5] * m[2] * m[11] - m[5] * m[3] * m[10] -
             m[9] * m[2] * m[7] + m[9] * m[3] * m[6];

    Inv[4] = -m[4] * m[10] * m[15] + m[4] * m[11] * m[14] + m[8] * m[6] * m[15] - m[8] * m[7] * m[14] -
             m[12] * m[6] * m[11] + m[12] * m[7] * m[10];
    Inv[5] = m[0] * m[10] * m[15] - m[0] * m[11] * m[14] - m[8] * m[2] * m[15] + m[8] * m[3] * m[14] +
             m[12] * m[2] * m[11] - m[12] * m[3] * m[10];
    Inv[6] = -m[0] * m[6] * m[15] + m[0] * m[7] * m[14] + m[4] * m[2] * m[15] - m[4] * m[3] * m[14] -
             m[12] * m[2] * m[7] + m[12] * m[3] * m[6];
    Inv[7] = m[0] * m[6] * m[11] - m[0] * m[7] * m[10] - m[4] * m[2] * m[11] + m[4] * m[3] * m[10] +
             m[8] * m[2] * m[7] - m[8] * m[3] * m[6];

    Inv[8] = m[4] * m[9] * m[15] - m[4] * m[11] * m[13] - m[8] * m[5] * m[15] + m[8] * m[7] * m[13] +
             m[12] * m[5] * m[11] - m[12] * m[7] * m[9];
    Inv[9] = -m[0] * m[9] * m[15] + m[0] * m[11] * m[13] + m[8] * m[1] * m[15] - m[8] * m[3] * m[13] -
             m[12] * m[1] * m[11] + m[12] * m[3] * m[9];
    Inv[10] = m[0] * m[5] * m[15] - m[0] * m[7] * m[13] - m[4] * m[1] * m[15] + m[4] * m[3] * m[13] +
              m[12] * m[1] * m[7] - m[12] * m[3] * m[5];
    Inv[11] = -m[0] * m[5] * m[11] + m[0] * m[7] * m[9] + m[4] * m[1] * m[11] - m[4] * m[3] * m[9] -
              m[8] * m[1] * m[7] + m[8] * m[3] * m[5];

    Inv[12] = -m[4] * m[9] * m[14] + m[4] * m[10] * m[13] + m[8] * m[5] * m[14] - m[8] * m[6] * m[13] -
              m[12] * m[5] * m[10] + m[12] * m[6] * m[9];
    Inv[13] = m[0] * m[9] * m[14] - m[0] * m[10] * m[13] - m[8] * m[1] * m[14] + m[8] * m[2] * m[13] +
              m[12] * m[1] * m[10] - m[12] * m[2] * m[9];
    Inv[14] = -m[0] * m[5] * m[14] + m[0] * m[6] * m[13] + m[4] * m[1] * m[14] - m[4] * m[2] * m[13] -
              m[12] * m[1] * m[6] + m[12] * m[2] * m[5];
    Inv[15] = m[0] * m[5] * m[10] - m[0] * m[6] * m[9] - m[4] * m[1] * m[10] + m[4] * m[2] * m[9] + m[8] * m[1] * m[6] -
              m[8] * m[2] * m[5];

    const float Det = m[0] * Inv[0] + m[1] * Inv[4] + m[2] * Inv[8] + m[3] * Inv[12];
    const float InvDet = 1.0f / Det;

    Mat4 R;
    R.Rows[0] = simd::mul(simd::load(Inv + 0), simd::splat(InvDet));
    R.Rows[1] = simd::mul(simd::load(Inv + 4), simd::splat(InvDet));
    R.Rows[2] = simd::mul(simd::load(Inv + 8), simd::splat(InvDet));
    R.Rows[3] = simd::mul(simd::load(Inv + 12), simd::splat(InvDet));
    return R;
}

Mat3 Mat3::rotationAxis(const Vec3 Axis, const float Radians)
{
    const Vec3 N = normalize(Axis);
    const float Nx = N.x(), Ny = N.y(), Nz = N.z();
    const float C = std::cos(Radians);
    const float S = std::sin(Radians);
    const float T = 1.0f - C;
    return {Vec3(T * Nx * Nx + C, T * Nx * Ny + S * Nz, T * Nx * Nz - S * Ny),
            Vec3(T * Nx * Ny - S * Nz, T * Ny * Ny + C, T * Ny * Nz + S * Nx),
            Vec3(T * Nx * Nz + S * Ny, T * Ny * Nz - S * Nx, T * Nz * Nz + C)};
}

float determinant(const Mat3& M)
{
    const Vec3 R0 = M.row(0), R1 = M.row(1), R2 = M.row(2);
    return dot(R0, cross(R1, R2));
}

Mat3 inverse(const Mat3& M)
{
    const Vec3 R0 = M.row(0), R1 = M.row(1), R2 = M.row(2);
    const Vec3 C0 = cross(R1, R2);
    const Vec3 C1 = cross(R2, R0);
    const Vec3 C2 = cross(R0, R1);
    const float InvDet = 1.0f / dot(R0, C0);
    const auto T = Mat3(C0, C1, C2);
    Mat3 R{};
    R.Rows[0] = simd::mul(T.Rows[0], simd::splat(InvDet));
    R.Rows[1] = simd::mul(T.Rows[1], simd::splat(InvDet));
    R.Rows[2] = simd::mul(T.Rows[2], simd::splat(InvDet));
    return transpose(R);
}

} // namespace goleta::math
