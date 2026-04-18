/// @file
/// @brief Out-of-line transform routines: toMatrix, composition, inverse, lerp.

#include "Math/Transform.h"

namespace goleta::math
{

Mat4 Transform::toMatrix() const
{
    const Vec3 R0 = rotate(Rotation, Vec3(Scale.x(), 0.0f, 0.0f));
    const Vec3 R1 = rotate(Rotation, Vec3(0.0f, Scale.y(), 0.0f));
    const Vec3 R2 = rotate(Rotation, Vec3(0.0f, 0.0f, Scale.z()));
    return {Vec4(R0.x(), R0.y(), R0.z(), 0.0f), Vec4(R1.x(), R1.y(), R1.z(), 0.0f), Vec4(R2.x(), R2.y(), R2.z(), 0.0f),
            Vec4(Translation.x(), Translation.y(), Translation.z(), 1.0f)};
}

Transform operator*(const Transform& Parent, const Transform& Child)
{
    return {Parent.Translation + rotate(Parent.Rotation, Parent.Scale * Child.Translation),
            Parent.Rotation * Child.Rotation, Parent.Scale * Child.Scale};
}

Transform inverse(const Transform& T)
{
    Quat InvRot = inverse(T.Rotation);
    auto InvScale = Vec3(1.0f / T.Scale.x(), 1.0f / T.Scale.y(), 1.0f / T.Scale.z());
    Vec3 InvTrans = rotate(InvRot, -T.Translation) * InvScale;
    return {InvTrans, InvRot, InvScale};
}

Transform lerp(const Transform& A, const Transform& B, const float T)
{
    return {lerp(A.Translation, B.Translation, T), nlerp(A.Rotation, B.Rotation, T), lerp(A.Scale, B.Scale, T)};
}

} // namespace goleta::math
