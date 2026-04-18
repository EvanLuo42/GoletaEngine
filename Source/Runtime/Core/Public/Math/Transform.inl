/// @file
/// @brief Inline Transform definitions; included from Transform.h.

namespace goleta::math
{

GOLETA_FORCEINLINE Transform::Transform(Vec3 T, Quat R, Vec3 S)
    : Translation(T)
    , Rotation(R)
    , Scale(S)
{
}

GOLETA_FORCEINLINE Transform Transform::identity()
{
    return Transform(Vec3::zero(), Quat::identity(), Vec3::one());
}

GOLETA_FORCEINLINE Vec3 transformPoint(const Transform& T, Vec3 P)
{
    return rotate(T.Rotation, P * T.Scale) + T.Translation;
}
GOLETA_FORCEINLINE Vec3 transformDirection(const Transform& T, Vec3 D)
{
    return rotate(T.Rotation, D * T.Scale);
}

} // namespace goleta::math
