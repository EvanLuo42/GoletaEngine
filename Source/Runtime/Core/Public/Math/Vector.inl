/// @file
/// @brief Inline definitions for Vec2 / Vec3 / Vec4; included from Vector.h.

#include <cmath>

namespace goleta::math
{

GOLETA_FORCEINLINE Vec4::Vec4(float X, float Y, float Z, float W)
    : Data(simd::set(X, Y, Z, W))
{
}
GOLETA_FORCEINLINE Vec4::Vec4(float X, float Y, float Z)
    : Data(simd::set(X, Y, Z, 0.0f))
{
}
GOLETA_FORCEINLINE Vec4::Vec4(float Splat)
    : Data(simd::splat(Splat))
{
}
GOLETA_FORCEINLINE Vec4::Vec4(simd::Vec4f V)
    : Data(V)
{
}

GOLETA_FORCEINLINE Vec4 Vec4::zero() { return Vec4(simd::zero()); }
GOLETA_FORCEINLINE Vec4 Vec4::one() { return Vec4(simd::one()); }
GOLETA_FORCEINLINE Vec4 Vec4::unitX() { return Vec4(1.0f, 0.0f, 0.0f, 0.0f); }
GOLETA_FORCEINLINE Vec4 Vec4::unitY() { return Vec4(0.0f, 1.0f, 0.0f, 0.0f); }
GOLETA_FORCEINLINE Vec4 Vec4::unitZ() { return Vec4(0.0f, 0.0f, 1.0f, 0.0f); }
GOLETA_FORCEINLINE Vec4 Vec4::unitW() { return Vec4(0.0f, 0.0f, 0.0f, 1.0f); }

GOLETA_FORCEINLINE float Vec4::x() const { return simd::getX(Data); }
GOLETA_FORCEINLINE float Vec4::y() const { return simd::getY(Data); }
GOLETA_FORCEINLINE float Vec4::z() const { return simd::getZ(Data); }
GOLETA_FORCEINLINE float Vec4::w() const { return simd::getW(Data); }

GOLETA_FORCEINLINE Vec4 operator+(Vec4 A, Vec4 B) { return Vec4(simd::add(A.Data, B.Data)); }
GOLETA_FORCEINLINE Vec4 operator-(Vec4 A, Vec4 B) { return Vec4(simd::sub(A.Data, B.Data)); }
GOLETA_FORCEINLINE Vec4 operator*(Vec4 A, Vec4 B) { return Vec4(simd::mul(A.Data, B.Data)); }
GOLETA_FORCEINLINE Vec4 operator/(Vec4 A, Vec4 B) { return Vec4(simd::div(A.Data, B.Data)); }
GOLETA_FORCEINLINE Vec4 operator*(Vec4 A, float S) { return Vec4(simd::mul(A.Data, simd::splat(S))); }
GOLETA_FORCEINLINE Vec4 operator*(float S, Vec4 A) { return Vec4(simd::mul(simd::splat(S), A.Data)); }
GOLETA_FORCEINLINE Vec4 operator-(Vec4 A) { return Vec4(simd::neg(A.Data)); }

GOLETA_FORCEINLINE Vec4& operator+=(Vec4& A, Vec4 B)
{
    A.Data = simd::add(A.Data, B.Data);
    return A;
}
GOLETA_FORCEINLINE Vec4& operator-=(Vec4& A, Vec4 B)
{
    A.Data = simd::sub(A.Data, B.Data);
    return A;
}
GOLETA_FORCEINLINE Vec4& operator*=(Vec4& A, Vec4 B)
{
    A.Data = simd::mul(A.Data, B.Data);
    return A;
}
GOLETA_FORCEINLINE Vec4& operator*=(Vec4& A, float S)
{
    A.Data = simd::mul(A.Data, simd::splat(S));
    return A;
}

inline bool operator==(Vec4 A, Vec4 B)
{
    return A.x() == B.x() && A.y() == B.y() && A.z() == B.z() && A.w() == B.w();
}
inline bool operator!=(Vec4 A, Vec4 B) { return !(A == B); }

GOLETA_FORCEINLINE float dot(Vec4 A, Vec4 B) { return simd::getX(simd::dot4(A.Data, B.Data)); }
GOLETA_FORCEINLINE float lengthSquared(Vec4 A) { return dot(A, A); }
GOLETA_FORCEINLINE float length(Vec4 A) { return std::sqrt(lengthSquared(A)); }
GOLETA_FORCEINLINE Vec4 normalize(Vec4 A)
{
    simd::Vec4f L2 = simd::dot4(A.Data, A.Data);
    return Vec4(simd::div(A.Data, simd::sqrt(L2)));
}
GOLETA_FORCEINLINE Vec4 min(Vec4 A, Vec4 B) { return Vec4(simd::min(A.Data, B.Data)); }
GOLETA_FORCEINLINE Vec4 max(Vec4 A, Vec4 B) { return Vec4(simd::max(A.Data, B.Data)); }
GOLETA_FORCEINLINE Vec4 lerp(Vec4 A, Vec4 B, float T)
{
    return Vec4(simd::fma(simd::sub(B.Data, A.Data), simd::splat(T), A.Data));
}
GOLETA_FORCEINLINE Vec4 abs(Vec4 A) { return Vec4(simd::abs(A.Data)); }

GOLETA_FORCEINLINE Vec3::Vec3(float X, float Y, float Z)
    : Data(simd::set(X, Y, Z, 0.0f))
{
}
GOLETA_FORCEINLINE Vec3::Vec3(float Splat)
    : Data(simd::set(Splat, Splat, Splat, 0.0f))
{
}
GOLETA_FORCEINLINE Vec3::Vec3(simd::Vec4f V)
    : Data(V)
{
}

GOLETA_FORCEINLINE Vec3 Vec3::zero() { return Vec3(simd::zero()); }
GOLETA_FORCEINLINE Vec3 Vec3::one() { return Vec3(1.0f); }
GOLETA_FORCEINLINE Vec3 Vec3::unitX() { return Vec3(1.0f, 0.0f, 0.0f); }
GOLETA_FORCEINLINE Vec3 Vec3::unitY() { return Vec3(0.0f, 1.0f, 0.0f); }
GOLETA_FORCEINLINE Vec3 Vec3::unitZ() { return Vec3(0.0f, 0.0f, 1.0f); }

GOLETA_FORCEINLINE float Vec3::x() const { return simd::getX(Data); }
GOLETA_FORCEINLINE float Vec3::y() const { return simd::getY(Data); }
GOLETA_FORCEINLINE float Vec3::z() const { return simd::getZ(Data); }

GOLETA_FORCEINLINE Vec3 operator+(Vec3 A, Vec3 B) { return Vec3(simd::add(A.Data, B.Data)); }
GOLETA_FORCEINLINE Vec3 operator-(Vec3 A, Vec3 B) { return Vec3(simd::sub(A.Data, B.Data)); }
GOLETA_FORCEINLINE Vec3 operator*(Vec3 A, Vec3 B) { return Vec3(simd::mul(A.Data, B.Data)); }
GOLETA_FORCEINLINE Vec3 operator*(Vec3 A, float S) { return Vec3(simd::mul(A.Data, simd::splat(S))); }
GOLETA_FORCEINLINE Vec3 operator*(float S, Vec3 A) { return Vec3(simd::mul(simd::splat(S), A.Data)); }
GOLETA_FORCEINLINE Vec3 operator-(Vec3 A) { return Vec3(simd::neg(A.Data)); }

GOLETA_FORCEINLINE Vec3& operator+=(Vec3& A, Vec3 B)
{
    A.Data = simd::add(A.Data, B.Data);
    return A;
}
GOLETA_FORCEINLINE Vec3& operator-=(Vec3& A, Vec3 B)
{
    A.Data = simd::sub(A.Data, B.Data);
    return A;
}
GOLETA_FORCEINLINE Vec3& operator*=(Vec3& A, float S)
{
    A.Data = simd::mul(A.Data, simd::splat(S));
    return A;
}

inline bool operator==(Vec3 A, Vec3 B) { return A.x() == B.x() && A.y() == B.y() && A.z() == B.z(); }
inline bool operator!=(Vec3 A, Vec3 B) { return !(A == B); }

GOLETA_FORCEINLINE float dot(Vec3 A, Vec3 B) { return simd::getX(simd::dot3(A.Data, B.Data)); }
GOLETA_FORCEINLINE Vec3 cross(Vec3 A, Vec3 B) { return Vec3(simd::cross3(A.Data, B.Data)); }
GOLETA_FORCEINLINE float lengthSquared(Vec3 A) { return dot(A, A); }
GOLETA_FORCEINLINE float length(Vec3 A) { return std::sqrt(lengthSquared(A)); }
GOLETA_FORCEINLINE Vec3 normalize(Vec3 A)
{
    simd::Vec4f L2 = simd::dot3(A.Data, A.Data);
    return Vec3(simd::div(A.Data, simd::sqrt(L2)));
}
GOLETA_FORCEINLINE Vec3 min(Vec3 A, Vec3 B) { return Vec3(simd::min(A.Data, B.Data)); }
GOLETA_FORCEINLINE Vec3 max(Vec3 A, Vec3 B) { return Vec3(simd::max(A.Data, B.Data)); }
GOLETA_FORCEINLINE Vec3 lerp(Vec3 A, Vec3 B, float T)
{
    return Vec3(simd::fma(simd::sub(B.Data, A.Data), simd::splat(T), A.Data));
}
GOLETA_FORCEINLINE Vec3 reflect(Vec3 Incident, Vec3 Normal) { return Incident - 2.0f * dot(Incident, Normal) * Normal; }

GOLETA_FORCEINLINE Vec2::Vec2(float X, float Y)
    : X(X)
    , Y(Y)
{
}
GOLETA_FORCEINLINE Vec2::Vec2(float Splat)
    : X(Splat)
    , Y(Splat)
{
}

GOLETA_FORCEINLINE Vec2 Vec2::zero() { return Vec2(0.0f, 0.0f); }
GOLETA_FORCEINLINE Vec2 Vec2::one() { return Vec2(1.0f, 1.0f); }
GOLETA_FORCEINLINE Vec2 Vec2::unitX() { return Vec2(1.0f, 0.0f); }
GOLETA_FORCEINLINE Vec2 Vec2::unitY() { return Vec2(0.0f, 1.0f); }

GOLETA_FORCEINLINE Vec2 operator+(Vec2 A, Vec2 B) { return Vec2(A.X + B.X, A.Y + B.Y); }
GOLETA_FORCEINLINE Vec2 operator-(Vec2 A, Vec2 B) { return Vec2(A.X - B.X, A.Y - B.Y); }
GOLETA_FORCEINLINE Vec2 operator*(Vec2 A, float S) { return Vec2(A.X * S, A.Y * S); }
GOLETA_FORCEINLINE Vec2 operator*(float S, Vec2 A) { return A * S; }
GOLETA_FORCEINLINE Vec2 operator-(Vec2 A) { return Vec2(-A.X, -A.Y); }

inline bool operator==(Vec2 A, Vec2 B) { return A.X == B.X && A.Y == B.Y; }
inline bool operator!=(Vec2 A, Vec2 B) { return !(A == B); }

GOLETA_FORCEINLINE float dot(Vec2 A, Vec2 B) { return A.X * B.X + A.Y * B.Y; }
GOLETA_FORCEINLINE float length(Vec2 A) { return std::sqrt(dot(A, A)); }
GOLETA_FORCEINLINE Vec2 normalize(Vec2 A)
{
    float L = length(A);
    return Vec2(A.X / L, A.Y / L);
}

} // namespace goleta::math
