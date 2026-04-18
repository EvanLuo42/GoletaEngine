#pragma once

/// @file
/// @brief Decomposed TRS affine transform (Translation, Rotation, Scale).

#include "CoreExport.h"
#include "Math/Matrix.h"
#include "Math/Quaternion.h"
#include "Math/Vector.h"

namespace goleta::math
{

/// @brief TRS affine transform stored as (Translation, Rotation, Scale).
/// @note Application order: Scale first, then Rotation, then Translation --
///       `transformPoint(T, P) = R(P * S) + Tr`. Inverse is only exact when
///       Scale is uniform.
struct GOLETA_ALIGN16 Transform
{
    Vec3 Translation;
    Quat Rotation;
    Vec3 Scale;

    /// @brief Leaves fields uninitialized.
    Transform() = default;
    /// @brief Constructs from explicit translation, rotation, and scale.
    Transform(Vec3 T, Quat R, Vec3 S);

    /// @brief Returns the identity transform (zero translation, identity rotation, unit scale).
    static Transform identity();

    /// @brief Returns the equivalent 4x4 matrix in row-vector convention.
    CORE_API Mat4 toMatrix() const;
};

/// @brief Composes child onto parent: `(Parent * Child) applied to P` equals
///        `Parent applied to (Child applied to P)`.
CORE_API Transform operator*(const Transform& Parent, const Transform& Child);

/// @brief Decomposed inverse. Exact only when Scale is uniform -- rotation does
///        not commute with non-uniform scale, so a round-trip with non-uniform
///        Scale will drift. For that case, invert via toMatrix() + inverse(Mat4).
CORE_API Transform inverse(const Transform& T);

/// @brief Transforms @p P as a point: applies scale, then rotation, then translation.
Vec3 transformPoint(const Transform& T, Vec3 P);
/// @brief Transforms @p D as a direction: applies scale and rotation but ignores translation.
Vec3 transformDirection(const Transform& T, Vec3 D);

/// @brief Componentwise blend: `lerp` on translation and scale, `nlerp` on rotation.
/// @note For constant angular velocity use a dedicated slerp-based transform blend instead.
CORE_API Transform lerp(const Transform& A, const Transform& B, float T);

} // namespace goleta::math

#include "Math/Transform.inl"
