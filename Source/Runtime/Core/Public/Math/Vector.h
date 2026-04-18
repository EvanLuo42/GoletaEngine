#pragma once

/// @file
/// @brief Object-oriented 2/3/4-float vector types backed by simd::Vec4f.

#include "Math/SIMD/SimdOps.h"

namespace goleta::math
{

/// @brief 4-component float vector backed by a 4-lane SIMD register.
struct GOLETA_ALIGN16 Vec4
{
    simd::Vec4f Data;

    /// @brief Leaves components uninitialized (SIMD default construction).
    Vec4() = default;
    /// @brief Constructs from explicit components.
    Vec4(float X, float Y, float Z, float W);
    /// @brief Constructs from X/Y/Z with W = 0.
    Vec4(float X, float Y, float Z);
    /// @brief Constructs with every lane set to @p Splat.
    explicit Vec4(float Splat);
    /// @brief Wraps a raw SIMD value.
    explicit Vec4(simd::Vec4f V);

    /// @brief Returns (0, 0, 0, 0).
    static Vec4 zero();
    /// @brief Returns (1, 1, 1, 1).
    static Vec4 one();
    /// @brief Returns the +X basis vector (1, 0, 0, 0).
    static Vec4 unitX();
    /// @brief Returns the +Y basis vector (0, 1, 0, 0).
    static Vec4 unitY();
    /// @brief Returns the +Z basis vector (0, 0, 1, 0).
    static Vec4 unitZ();
    /// @brief Returns the +W basis vector (0, 0, 0, 1).
    static Vec4 unitW();

    /// @brief Returns the X component.
    float x() const;
    /// @brief Returns the Y component.
    float y() const;
    /// @brief Returns the Z component.
    float z() const;
    /// @brief Returns the W component.
    float w() const;
};

/// @brief Componentwise addition.
Vec4 operator+(Vec4 A, Vec4 B);
/// @brief Componentwise subtraction.
Vec4 operator-(Vec4 A, Vec4 B);
/// @brief Componentwise (Hadamard) multiplication.
Vec4 operator*(Vec4 A, Vec4 B);
/// @brief Componentwise division. @note Undefined when any lane of @p B is 0.
Vec4 operator/(Vec4 A, Vec4 B);
/// @brief Scales every component by @p S.
Vec4 operator*(Vec4 A, float S);
/// @brief Scales every component by @p S.
Vec4 operator*(float S, Vec4 A);
/// @brief Componentwise negation.
Vec4 operator-(Vec4 A);

/// @brief In-place componentwise addition.
Vec4& operator+=(Vec4& A, Vec4 B);
/// @brief In-place componentwise subtraction.
Vec4& operator-=(Vec4& A, Vec4 B);
/// @brief In-place componentwise multiplication.
Vec4& operator*=(Vec4& A, Vec4 B);
/// @brief In-place scalar multiplication.
Vec4& operator*=(Vec4& A, float S);

/// @brief Exact componentwise equality. @note Uses `==` on each lane; not suitable for values
///        produced by floating-point arithmetic that may drift by 1 ULP.
bool operator==(Vec4 A, Vec4 B);
/// @brief Inverse of `operator==`.
bool operator!=(Vec4 A, Vec4 B);

/// @brief Returns the 4D dot product A · B.
float dot(Vec4 A, Vec4 B);
/// @brief Returns |A|^2. Prefer this over `length` when the square root is unnecessary.
float lengthSquared(Vec4 A);
/// @brief Returns the Euclidean length |A|.
float length(Vec4 A);
/// @brief Returns A scaled to unit length. @note Undefined when |A| is 0.
Vec4 normalize(Vec4 A);
/// @brief Componentwise minimum.
Vec4 min(Vec4 A, Vec4 B);
/// @brief Componentwise maximum.
Vec4 max(Vec4 A, Vec4 B);
/// @brief Linear interpolation: A + T * (B - A). @p T is unclamped.
Vec4 lerp(Vec4 A, Vec4 B, float T);
/// @brief Componentwise absolute value.
Vec4 abs(Vec4 A);

/// @brief 3D vector. Backed by a 4-lane SIMD value with lane W held at 0.
struct GOLETA_ALIGN16 Vec3
{
    simd::Vec4f Data;

    /// @brief Leaves components uninitialized.
    Vec3() = default;
    /// @brief Constructs from explicit components; W is set to 0.
    Vec3(float X, float Y, float Z);
    /// @brief Constructs with every component set to @p Splat; W is 0.
    explicit Vec3(float Splat);
    /// @brief Wraps a raw SIMD value. @note Lane W is expected to be 0.
    explicit Vec3(simd::Vec4f V);

    /// @brief Returns (0, 0, 0).
    static Vec3 zero();
    /// @brief Returns (1, 1, 1).
    static Vec3 one();
    /// @brief Returns the +X basis vector.
    static Vec3 unitX();
    /// @brief Returns the +Y basis vector.
    static Vec3 unitY();
    /// @brief Returns the +Z basis vector.
    static Vec3 unitZ();

    /// @brief Returns the X component.
    float x() const;
    /// @brief Returns the Y component.
    float y() const;
    /// @brief Returns the Z component.
    float z() const;
};

/// @brief Componentwise addition.
Vec3 operator+(Vec3 A, Vec3 B);
/// @brief Componentwise subtraction.
Vec3 operator-(Vec3 A, Vec3 B);
/// @brief Componentwise (Hadamard) multiplication.
Vec3 operator*(Vec3 A, Vec3 B);
/// @brief Scales every component by @p S.
Vec3 operator*(Vec3 A, float S);
/// @brief Scales every component by @p S.
Vec3 operator*(float S, Vec3 A);
/// @brief Componentwise negation.
Vec3 operator-(Vec3 A);

/// @brief In-place componentwise addition.
Vec3& operator+=(Vec3& A, Vec3 B);
/// @brief In-place componentwise subtraction.
Vec3& operator-=(Vec3& A, Vec3 B);
/// @brief In-place scalar multiplication.
Vec3& operator*=(Vec3& A, float S);

/// @brief Exact componentwise equality. @note Compares only X/Y/Z; W is ignored.
bool operator==(Vec3 A, Vec3 B);
/// @brief Inverse of `operator==`.
bool operator!=(Vec3 A, Vec3 B);

/// @brief Returns the 3D dot product A · B.
float dot(Vec3 A, Vec3 B);
/// @brief Returns the right-handed cross product A × B.
Vec3 cross(Vec3 A, Vec3 B);
/// @brief Returns |A|^2. Prefer this over `length` when the square root is unnecessary.
float lengthSquared(Vec3 A);
/// @brief Returns the Euclidean length |A|.
float length(Vec3 A);
/// @brief Returns A scaled to unit length. @note Undefined when |A| is 0.
Vec3 normalize(Vec3 A);
/// @brief Componentwise minimum.
Vec3 min(Vec3 A, Vec3 B);
/// @brief Componentwise maximum.
Vec3 max(Vec3 A, Vec3 B);
/// @brief Linear interpolation: A + T * (B - A). @p T is unclamped.
Vec3 lerp(Vec3 A, Vec3 B, float T);
/// @brief Reflects @p Incident across the plane defined by unit @p Normal.
///        @note @p Normal is assumed unit length.
Vec3 reflect(Vec3 Incident, Vec3 Normal);

/// @brief 2D vector stored as two plain floats (not SIMD).
struct Vec2
{
    float X;
    float Y;

    /// @brief Leaves components uninitialized.
    Vec2() = default;
    /// @brief Constructs from explicit components.
    Vec2(float X, float Y);
    /// @brief Constructs with both components set to @p Splat.
    explicit Vec2(float Splat);

    /// @brief Returns (0, 0).
    static Vec2 zero();
    /// @brief Returns (1, 1).
    static Vec2 one();
    /// @brief Returns the +X basis vector.
    static Vec2 unitX();
    /// @brief Returns the +Y basis vector.
    static Vec2 unitY();
};

/// @brief Componentwise addition.
Vec2 operator+(Vec2 A, Vec2 B);
/// @brief Componentwise subtraction.
Vec2 operator-(Vec2 A, Vec2 B);
/// @brief Scales both components by @p S.
Vec2 operator*(Vec2 A, float S);
/// @brief Scales both components by @p S.
Vec2 operator*(float S, Vec2 A);
/// @brief Componentwise negation.
Vec2 operator-(Vec2 A);

/// @brief Exact componentwise equality.
bool operator==(Vec2 A, Vec2 B);
/// @brief Inverse of `operator==`.
bool operator!=(Vec2 A, Vec2 B);

/// @brief Returns the 2D dot product A · B.
float dot(Vec2 A, Vec2 B);
/// @brief Returns the Euclidean length |A|.
float length(Vec2 A);
/// @brief Returns A scaled to unit length. @note Undefined when |A| is 0.
Vec2 normalize(Vec2 A);

} // namespace goleta::math

#include "Math/Vector.inl"
