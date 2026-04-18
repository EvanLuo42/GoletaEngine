/// @file
/// @brief Out-of-line quaternion routines: fromEuler, fromRotationMatrix, slerp.

#include "Math/Quaternion.h"

#include <cmath>

#include "Math/Matrix.h"

namespace goleta::math
{

// Pitch around X, Yaw around Y, Roll around Z; applied in Z-Y-X order so a
// vector is rolled first, then yawed, then pitched.
Quat Quat::fromEuler(const float Pitch, const float Yaw, const float Roll)
{
    const float Hp = Pitch * 0.5f, Hy = Yaw * 0.5f, Hr = Roll * 0.5f;
    const float Cp = std::cos(Hp), Sp = std::sin(Hp);
    const float Cy = std::cos(Hy), Sy = std::sin(Hy);
    const float Cr = std::cos(Hr), Sr = std::sin(Hr);
    return {Sp * Cy * Cr - Cp * Sy * Sr, Cp * Sy * Cr + Sp * Cy * Sr, Cp * Cy * Sr - Sp * Sy * Cr,
            Cp * Cy * Cr + Sp * Sy * Sr};
}

// Shepperd's method: pick the largest diagonal term for numerical stability.
Quat Quat::fromRotationMatrix(const Mat3& M)
{
    alignas(16) float m[12];
    simd::store(m + 0, M.Rows[0]);
    simd::store(m + 4, M.Rows[1]);
    simd::store(m + 8, M.Rows[2]);

    if (const float Trace = m[0] + m[5] + m[10]; Trace > 0.0f)
    {
        const float S = std::sqrt(Trace + 1.0f) * 2.0f;
        return {(m[6] - m[9]) / S, (m[8] - m[2]) / S, (m[1] - m[4]) / S, 0.25f * S};
    }
    if (m[0] > m[5] && m[0] > m[10])
    {
        const float S = std::sqrt(1.0f + m[0] - m[5] - m[10]) * 2.0f;
        return {0.25f * S, (m[4] + m[1]) / S, (m[8] + m[2]) / S, (m[6] - m[9]) / S};
    }
    if (m[5] > m[10])
    {
        const float S = std::sqrt(1.0f + m[5] - m[0] - m[10]) * 2.0f;
        return {(m[4] + m[1]) / S, 0.25f * S, (m[9] + m[6]) / S, (m[8] - m[2]) / S};
    }
    const float S = std::sqrt(1.0f + m[10] - m[0] - m[5]) * 2.0f;
    return {(m[8] + m[2]) / S, (m[9] + m[6]) / S, 0.25f * S, (m[1] - m[4]) / S};
}

Quat slerp(const Quat A, const Quat B, const float T)
{
    float Cos = dot(A, B);
    simd::Vec4f BSigned = B.Data;
    if (Cos < 0.0f)
    {
        BSigned = simd::neg(BSigned);
        Cos = -Cos;
    }
    // Above this threshold the angle is too small for sin() to be well-
    // conditioned; fall back to a normalized lerp to avoid division by ~0.
    if (constexpr float SlerpEpsilon = 0.9995f; Cos > SlerpEpsilon)
        return nlerp(A, Quat(BSigned), T);

    const float Angle = std::acos(Cos);
    const float SinA = std::sin(Angle);
    const float Wa = std::sin((1.0f - T) * Angle) / SinA;
    const float Wb = std::sin(T * Angle) / SinA;

    simd::Vec4f R = simd::add(simd::mul(A.Data, simd::splat(Wa)), simd::mul(BSigned, simd::splat(Wb)));
    return Quat(R);
}

} // namespace goleta::math
