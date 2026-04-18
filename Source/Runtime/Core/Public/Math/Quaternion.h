#pragma once

/// @file
/// @brief Unit quaternion for 3D rotation. Stored as (X, Y, Z, W).

#include "CoreExport.h"
#include "Math/SIMD/SimdOps.h"
#include "Math/Vector.h"

namespace goleta::math
{

/// @brief Unit quaternion representing a 3D rotation. @note Most routines
///        (rotate / slerp / inverse) assume the quaternion is unit length;
///        use `normalize` after arithmetic that breaks that invariant.
struct GOLETA_ALIGN16 Quat
{
    simd::Vec4f Data; // (X, Y, Z, W)

    /// @brief Leaves components uninitialized.
    Quat() = default;
    /// @brief Constructs from explicit components (X, Y, Z, W).
    Quat(float X, float Y, float Z, float W);
    /// @brief Wraps a raw SIMD value in (X, Y, Z, W) layout.
    explicit Quat(simd::Vec4f V);

    /// @brief Returns the identity rotation (0, 0, 0, 1).
    static Quat identity();
    /// @brief Builds a unit quaternion from axis and angle.
    /// @param Axis Rotation axis; normalized internally.
    /// @param Radians Angle in radians; positive is right-handed around @p Axis.
    static Quat fromAxisAngle(Vec3 Axis, float Radians);
    /// @brief Builds a unit quaternion from Euler angles.
    /// @param Pitch Rotation around X (radians).
    /// @param Yaw Rotation around Y (radians).
    /// @param Roll Rotation around Z (radians).
    /// @note Applied in Z-Y-X order: roll first, then yaw, then pitch.
    static CORE_API Quat fromEuler(float Pitch, float Yaw, float Roll);
    /// @brief Extracts the rotation encoded by an orthonormal 3x3 matrix.
    /// @note Uses Shepperd's method (largest-diagonal branch) for numerical stability;
    ///       expects @p M to be a proper rotation (orthonormal, det = +1).
    static CORE_API Quat fromRotationMatrix(const struct Mat3& M);

    /// @brief Returns the X component.
    float x() const;
    /// @brief Returns the Y component.
    float y() const;
    /// @brief Returns the Z component.
    float z() const;
    /// @brief Returns the W (scalar) component.
    float w() const;
};

/// @brief Componentwise addition. @note Result is generally not a unit quaternion.
Quat operator+(Quat A, Quat B);
/// @brief Componentwise subtraction. @note Result is generally not a unit quaternion.
Quat operator-(Quat A, Quat B);
/// @brief Hamilton product: `rotate(A*B, v) == rotate(A, rotate(B, v))`.
Quat operator*(Quat A, Quat B);
/// @brief Scales every component by @p S. @note Result is generally not a unit quaternion.
Quat operator*(Quat Q, float S);
/// @brief Componentwise negation. @note `-Q` and `Q` represent the same 3D rotation.
Quat operator-(Quat Q);

/// @brief Exact componentwise equality.
/// @note Two unit quaternions that represent the same rotation but differ in overall
///       sign (`Q` vs `-Q`) compare as not-equal.
bool operator==(Quat A, Quat B);
/// @brief Inverse of `operator==`.
bool operator!=(Quat A, Quat B);

/// @brief 4D dot product. For unit quats this equals `cos(half-angle between them)`.
float dot(Quat A, Quat B);
/// @brief Returns the Euclidean norm |Q|.
float length(Quat Q);
/// @brief Returns Q scaled to unit length. @note Undefined when |Q| is 0.
Quat normalize(Quat Q);
/// @brief Returns (-X, -Y, -Z, W). For unit quats this equals the inverse rotation.
Quat conjugate(Quat Q);
/// @brief Returns the rotation inverse. @note Valid only for unit quaternions.
Quat inverse(Quat Q);

/// @brief Rotates @p V by unit quaternion @p Q (formula: V + 2 * cross(U, cross(U,V) + w*V)).
Vec3 rotate(Quat Q, Vec3 V);
/// @brief Spherical linear interpolation along the short arc.
/// @note Flips @p B's sign when dot(A,B) < 0 to take the short arc; falls back to
///       `nlerp` when the angle is tiny (dot > 0.9995) to avoid sin-near-zero division.
CORE_API Quat slerp(Quat A, Quat B, float T);
/// @brief Normalized linear interpolation; cheaper than `slerp` and short-arc-safe.
/// @note Output is unit length but interpolation is not constant angular velocity.
Quat nlerp(Quat A, Quat B, float T);

} // namespace goleta::math

#include "Math/Quaternion.inl"
