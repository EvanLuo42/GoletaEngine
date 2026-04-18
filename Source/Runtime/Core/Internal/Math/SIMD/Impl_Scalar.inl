/// @file
/// @brief Reference scalar SIMD backend. Serves as the numeric golden for
///        cross-backend tests; correctness over speed.

#include <bit>
#include <cmath>
#include <cstdint>

namespace goleta::simd
{
namespace detail
{

GOLETA_FORCEINLINE float maskLaneFromBool(bool B) { return std::bit_cast<float>(B ? 0xFFFFFFFFu : 0u); }

GOLETA_FORCEINLINE float bitAnd(float A, float B)
{
    return std::bit_cast<float>(std::bit_cast<uint32_t>(A) & std::bit_cast<uint32_t>(B));
}
GOLETA_FORCEINLINE float bitOr(float A, float B)
{
    return std::bit_cast<float>(std::bit_cast<uint32_t>(A) | std::bit_cast<uint32_t>(B));
}
GOLETA_FORCEINLINE float bitXor(float A, float B)
{
    return std::bit_cast<float>(std::bit_cast<uint32_t>(A) ^ std::bit_cast<uint32_t>(B));
}

} // namespace detail

GOLETA_FORCEINLINE Vec4f load(const float* P) { return Vec4f{{P[0], P[1], P[2], P[3]}}; }
GOLETA_FORCEINLINE Vec4f loadU(const float* P) { return Vec4f{{P[0], P[1], P[2], P[3]}}; }
GOLETA_FORCEINLINE void store(float* P, Vec4f V)
{
    P[0] = V.V[0];
    P[1] = V.V[1];
    P[2] = V.V[2];
    P[3] = V.V[3];
}
GOLETA_FORCEINLINE void storeU(float* P, Vec4f V) { store(P, V); }

GOLETA_FORCEINLINE Vec4f set(float X, float Y, float Z, float W) { return Vec4f{{X, Y, Z, W}}; }
GOLETA_FORCEINLINE Vec4f splat(float V) { return Vec4f{{V, V, V, V}}; }
GOLETA_FORCEINLINE Vec4f zero() { return splat(0.0f); }
GOLETA_FORCEINLINE Vec4f one() { return splat(1.0f); }

GOLETA_FORCEINLINE Vec4f add(Vec4f A, Vec4f B)
{
    return Vec4f{{A.V[0] + B.V[0], A.V[1] + B.V[1], A.V[2] + B.V[2], A.V[3] + B.V[3]}};
}
GOLETA_FORCEINLINE Vec4f sub(Vec4f A, Vec4f B)
{
    return Vec4f{{A.V[0] - B.V[0], A.V[1] - B.V[1], A.V[2] - B.V[2], A.V[3] - B.V[3]}};
}
GOLETA_FORCEINLINE Vec4f mul(Vec4f A, Vec4f B)
{
    return Vec4f{{A.V[0] * B.V[0], A.V[1] * B.V[1], A.V[2] * B.V[2], A.V[3] * B.V[3]}};
}
GOLETA_FORCEINLINE Vec4f div(Vec4f A, Vec4f B)
{
    return Vec4f{{A.V[0] / B.V[0], A.V[1] / B.V[1], A.V[2] / B.V[2], A.V[3] / B.V[3]}};
}

// Scalar fma is not fused -- two rounding steps. The SSE/NEON backends use
// hardware FMA, so cross-backend tests must allow 1-2 ULP of difference.
GOLETA_FORCEINLINE Vec4f fma(Vec4f A, Vec4f B, Vec4f C) { return add(mul(A, B), C); }
GOLETA_FORCEINLINE Vec4f fms(Vec4f A, Vec4f B, Vec4f C) { return sub(mul(A, B), C); }

GOLETA_FORCEINLINE Vec4f neg(Vec4f A) { return Vec4f{{-A.V[0], -A.V[1], -A.V[2], -A.V[3]}}; }
GOLETA_FORCEINLINE Vec4f abs(Vec4f A)
{
    return Vec4f{{std::fabs(A.V[0]), std::fabs(A.V[1]), std::fabs(A.V[2]), std::fabs(A.V[3])}};
}
GOLETA_FORCEINLINE Vec4f min(Vec4f A, Vec4f B)
{
    return Vec4f{
        {std::fmin(A.V[0], B.V[0]), std::fmin(A.V[1], B.V[1]), std::fmin(A.V[2], B.V[2]), std::fmin(A.V[3], B.V[3])}};
}
GOLETA_FORCEINLINE Vec4f max(Vec4f A, Vec4f B)
{
    return Vec4f{
        {std::fmax(A.V[0], B.V[0]), std::fmax(A.V[1], B.V[1]), std::fmax(A.V[2], B.V[2]), std::fmax(A.V[3], B.V[3])}};
}

GOLETA_FORCEINLINE Vec4f sqrt(Vec4f A)
{
    return Vec4f{{std::sqrt(A.V[0]), std::sqrt(A.V[1]), std::sqrt(A.V[2]), std::sqrt(A.V[3])}};
}
GOLETA_FORCEINLINE Vec4f rsqrt(Vec4f A)
{
    return Vec4f{
        {1.0f / std::sqrt(A.V[0]), 1.0f / std::sqrt(A.V[1]), 1.0f / std::sqrt(A.V[2]), 1.0f / std::sqrt(A.V[3])}};
}
GOLETA_FORCEINLINE Vec4f rcp(Vec4f A) { return Vec4f{{1.0f / A.V[0], 1.0f / A.V[1], 1.0f / A.V[2], 1.0f / A.V[3]}}; }

GOLETA_FORCEINLINE Vec4f dot3(Vec4f A, Vec4f B)
{
    float D = A.V[0] * B.V[0] + A.V[1] * B.V[1] + A.V[2] * B.V[2];
    return splat(D);
}
GOLETA_FORCEINLINE Vec4f dot4(Vec4f A, Vec4f B)
{
    float D = A.V[0] * B.V[0] + A.V[1] * B.V[1] + A.V[2] * B.V[2] + A.V[3] * B.V[3];
    return splat(D);
}
GOLETA_FORCEINLINE Vec4f cross3(Vec4f A, Vec4f B)
{
    return Vec4f{{A.V[1] * B.V[2] - A.V[2] * B.V[1], A.V[2] * B.V[0] - A.V[0] * B.V[2],
                  A.V[0] * B.V[1] - A.V[1] * B.V[0], 0.0f}};
}

GOLETA_FORCEINLINE Vec4f cmpEq(Vec4f A, Vec4f B)
{
    return Vec4f{{detail::maskLaneFromBool(A.V[0] == B.V[0]), detail::maskLaneFromBool(A.V[1] == B.V[1]),
                  detail::maskLaneFromBool(A.V[2] == B.V[2]), detail::maskLaneFromBool(A.V[3] == B.V[3])}};
}
GOLETA_FORCEINLINE Vec4f cmpNe(Vec4f A, Vec4f B)
{
    return Vec4f{{detail::maskLaneFromBool(A.V[0] != B.V[0]), detail::maskLaneFromBool(A.V[1] != B.V[1]),
                  detail::maskLaneFromBool(A.V[2] != B.V[2]), detail::maskLaneFromBool(A.V[3] != B.V[3])}};
}
GOLETA_FORCEINLINE Vec4f cmpLt(Vec4f A, Vec4f B)
{
    return Vec4f{{detail::maskLaneFromBool(A.V[0] < B.V[0]), detail::maskLaneFromBool(A.V[1] < B.V[1]),
                  detail::maskLaneFromBool(A.V[2] < B.V[2]), detail::maskLaneFromBool(A.V[3] < B.V[3])}};
}
GOLETA_FORCEINLINE Vec4f cmpLe(Vec4f A, Vec4f B)
{
    return Vec4f{{detail::maskLaneFromBool(A.V[0] <= B.V[0]), detail::maskLaneFromBool(A.V[1] <= B.V[1]),
                  detail::maskLaneFromBool(A.V[2] <= B.V[2]), detail::maskLaneFromBool(A.V[3] <= B.V[3])}};
}
GOLETA_FORCEINLINE Vec4f cmpGt(Vec4f A, Vec4f B)
{
    return Vec4f{{detail::maskLaneFromBool(A.V[0] > B.V[0]), detail::maskLaneFromBool(A.V[1] > B.V[1]),
                  detail::maskLaneFromBool(A.V[2] > B.V[2]), detail::maskLaneFromBool(A.V[3] > B.V[3])}};
}
GOLETA_FORCEINLINE Vec4f cmpGe(Vec4f A, Vec4f B)
{
    return Vec4f{{detail::maskLaneFromBool(A.V[0] >= B.V[0]), detail::maskLaneFromBool(A.V[1] >= B.V[1]),
                  detail::maskLaneFromBool(A.V[2] >= B.V[2]), detail::maskLaneFromBool(A.V[3] >= B.V[3])}};
}

GOLETA_FORCEINLINE Vec4f andMask(Vec4f A, Vec4f B)
{
    return Vec4f{{detail::bitAnd(A.V[0], B.V[0]), detail::bitAnd(A.V[1], B.V[1]), detail::bitAnd(A.V[2], B.V[2]),
                  detail::bitAnd(A.V[3], B.V[3])}};
}
GOLETA_FORCEINLINE Vec4f orMask(Vec4f A, Vec4f B)
{
    return Vec4f{{detail::bitOr(A.V[0], B.V[0]), detail::bitOr(A.V[1], B.V[1]), detail::bitOr(A.V[2], B.V[2]),
                  detail::bitOr(A.V[3], B.V[3])}};
}
GOLETA_FORCEINLINE Vec4f xorMask(Vec4f A, Vec4f B)
{
    return Vec4f{{detail::bitXor(A.V[0], B.V[0]), detail::bitXor(A.V[1], B.V[1]), detail::bitXor(A.V[2], B.V[2]),
                  detail::bitXor(A.V[3], B.V[3])}};
}
GOLETA_FORCEINLINE Vec4f andNot(Vec4f A, Vec4f B)
{
    using std::bit_cast;
    return Vec4f{{bit_cast<float>(~bit_cast<uint32_t>(A.V[0]) & bit_cast<uint32_t>(B.V[0])),
                  bit_cast<float>(~bit_cast<uint32_t>(A.V[1]) & bit_cast<uint32_t>(B.V[1])),
                  bit_cast<float>(~bit_cast<uint32_t>(A.V[2]) & bit_cast<uint32_t>(B.V[2])),
                  bit_cast<float>(~bit_cast<uint32_t>(A.V[3]) & bit_cast<uint32_t>(B.V[3]))}};
}
GOLETA_FORCEINLINE Vec4f select(Vec4f Mask, Vec4f A, Vec4f B) { return orMask(andMask(Mask, A), andNot(Mask, B)); }

template <int X, int Y, int Z, int W>
GOLETA_FORCEINLINE Vec4f shuffle(Vec4f A)
{
    return Vec4f{{A.V[X], A.V[Y], A.V[Z], A.V[W]}};
}
template <int X, int Y, int Z, int W>
GOLETA_FORCEINLINE Vec4f shuffle2(Vec4f A, Vec4f B)
{
    return Vec4f{{A.V[X], A.V[Y], B.V[Z], B.V[W]}};
}

GOLETA_FORCEINLINE float getX(Vec4f A) { return A.V[0]; }
GOLETA_FORCEINLINE float getY(Vec4f A) { return A.V[1]; }
GOLETA_FORCEINLINE float getZ(Vec4f A) { return A.V[2]; }
GOLETA_FORCEINLINE float getW(Vec4f A) { return A.V[3]; }

GOLETA_FORCEINLINE Vec4f splatX(Vec4f A) { return splat(A.V[0]); }
GOLETA_FORCEINLINE Vec4f splatY(Vec4f A) { return splat(A.V[1]); }
GOLETA_FORCEINLINE Vec4f splatZ(Vec4f A) { return splat(A.V[2]); }
GOLETA_FORCEINLINE Vec4f splatW(Vec4f A) { return splat(A.V[3]); }

} // namespace goleta::simd
