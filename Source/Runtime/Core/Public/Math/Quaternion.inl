/// @file
/// @brief Inline Quat definitions; included from Quaternion.h. Heavy routines
///        (slerp, fromEuler, fromRotationMatrix) live in Quaternion.cpp.

#pragma once

#include <cmath>

namespace goleta::math
{

GOLETA_FORCEINLINE Quat::Quat(float X, float Y, float Z, float W)
    : Data(simd::set(X, Y, Z, W))
{
}
GOLETA_FORCEINLINE Quat::Quat(simd::Vec4f V)
    : Data(V)
{
}

GOLETA_FORCEINLINE Quat Quat::identity() { return Quat(0.0f, 0.0f, 0.0f, 1.0f); }

GOLETA_FORCEINLINE Quat Quat::fromAxisAngle(const Vec3 Axis, const float Radians)
{
    float Half = Radians * 0.5f;
    float S = std::sin(Half);
    float C = std::cos(Half);
    Vec3 N = normalize(Axis);
    return Quat(N.x() * S, N.y() * S, N.z() * S, C);
}

GOLETA_FORCEINLINE float Quat::x() const { return simd::getX(Data); }
GOLETA_FORCEINLINE float Quat::y() const { return simd::getY(Data); }
GOLETA_FORCEINLINE float Quat::z() const { return simd::getZ(Data); }
GOLETA_FORCEINLINE float Quat::w() const { return simd::getW(Data); }

GOLETA_FORCEINLINE Quat operator+(Quat A, Quat B) { return Quat(simd::add(A.Data, B.Data)); }
GOLETA_FORCEINLINE Quat operator-(Quat A, Quat B) { return Quat(simd::sub(A.Data, B.Data)); }
GOLETA_FORCEINLINE Quat operator*(Quat Q, float S) { return Quat(simd::mul(Q.Data, simd::splat(S))); }
GOLETA_FORCEINLINE Quat operator-(Quat Q) { return Quat(simd::neg(Q.Data)); }

// Hamilton product. Written as scalar arithmetic for clarity -- the compiler
// generates reasonable code, and a hand-vectorized version is a micro-opt we
// can do later if profiling calls for it.
GOLETA_FORCEINLINE Quat operator*(Quat A, Quat B)
{
    const float Ax = A.x(), Ay = A.y(), Az = A.z(), Aw = A.w();
    const float Bx = B.x(), By = B.y(), Bz = B.z(), Bw = B.w();
    return Quat(Aw * Bx + Ax * Bw + Ay * Bz - Az * By, Aw * By - Ax * Bz + Ay * Bw + Az * Bx,
                Aw * Bz + Ax * By - Ay * Bx + Az * Bw, Aw * Bw - Ax * Bx - Ay * By - Az * Bz);
}

inline bool operator==(Quat A, Quat B) { return A.x() == B.x() && A.y() == B.y() && A.z() == B.z() && A.w() == B.w(); }
inline bool operator!=(Quat A, Quat B) { return !(A == B); }

GOLETA_FORCEINLINE float dot(Quat A, Quat B) { return simd::getX(simd::dot4(A.Data, B.Data)); }
GOLETA_FORCEINLINE float length(Quat Q) { return std::sqrt(dot(Q, Q)); }
GOLETA_FORCEINLINE Quat normalize(Quat Q)
{
    simd::Vec4f L2 = simd::dot4(Q.Data, Q.Data);
    return Quat(simd::div(Q.Data, simd::sqrt(L2)));
}
GOLETA_FORCEINLINE Quat conjugate(Quat Q) { return Quat(-Q.x(), -Q.y(), -Q.z(), Q.w()); }
GOLETA_FORCEINLINE Quat inverse(Quat Q) { return conjugate(Q); } // valid for unit quaternions

GOLETA_FORCEINLINE Vec3 rotate(Quat Q, Vec3 V)
{
    Vec3 U(Q.x(), Q.y(), Q.z());
    Vec3 T = 2.0f * cross(U, V);
    return V + Q.w() * T + cross(U, T);
}

GOLETA_FORCEINLINE Quat nlerp(Quat A, Quat B, float T)
{
    float D = dot(A, B);
    simd::Vec4f BSigned = D < 0.0f ? simd::neg(B.Data) : B.Data;
    simd::Vec4f Lerped = simd::fma(simd::sub(BSigned, A.Data), simd::splat(T), A.Data);
    return normalize(Quat(Lerped));
}

} // namespace goleta::math
