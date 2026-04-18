#pragma once

/// @file
/// @brief Cross-platform SIMD operation interface. Free functions on opaque
///        Vec4f / Vec4i so values stay in SIMD registers across calls.

#include "Math/SIMD/SimdTypes.h"

namespace goleta::simd
{

/// @brief Loads four floats from 16-byte-aligned memory.
GOLETA_FORCEINLINE Vec4f load(const float* P);
/// @brief Loads four floats from possibly unaligned memory.
GOLETA_FORCEINLINE Vec4f loadU(const float* P);
/// @brief Stores @p V to 16-byte-aligned memory.
GOLETA_FORCEINLINE void store(float* P, Vec4f V);
/// @brief Stores @p V to possibly unaligned memory.
GOLETA_FORCEINLINE void storeU(float* P, Vec4f V);

/// @brief Builds a 4-lane value from explicit components.
GOLETA_FORCEINLINE Vec4f set(float X, float Y, float Z, float W);
/// @brief Broadcasts @p V into every lane.
GOLETA_FORCEINLINE Vec4f splat(float V);
/// @brief Returns (0, 0, 0, 0).
GOLETA_FORCEINLINE Vec4f zero();
/// @brief Returns (1, 1, 1, 1).
GOLETA_FORCEINLINE Vec4f one();

/// @brief Lanewise A + B.
GOLETA_FORCEINLINE Vec4f add(Vec4f A, Vec4f B);
/// @brief Lanewise A - B.
GOLETA_FORCEINLINE Vec4f sub(Vec4f A, Vec4f B);
/// @brief Lanewise A * B.
GOLETA_FORCEINLINE Vec4f mul(Vec4f A, Vec4f B);
/// @brief Lanewise A / B. @note Undefined when any lane of @p B is 0.
GOLETA_FORCEINLINE Vec4f div(Vec4f A, Vec4f B);
/// @brief Fused multiply-add: A * B + C (single rounding step where hardware supports it).
GOLETA_FORCEINLINE Vec4f fma(Vec4f A, Vec4f B, Vec4f C);
/// @brief Fused multiply-subtract: A * B - C.
GOLETA_FORCEINLINE Vec4f fms(Vec4f A, Vec4f B, Vec4f C);

/// @brief Lanewise unary negation.
GOLETA_FORCEINLINE Vec4f neg(Vec4f A);
/// @brief Lanewise absolute value.
GOLETA_FORCEINLINE Vec4f abs(Vec4f A);
/// @brief Lanewise minimum.
GOLETA_FORCEINLINE Vec4f min(Vec4f A, Vec4f B);
/// @brief Lanewise maximum.
GOLETA_FORCEINLINE Vec4f max(Vec4f A, Vec4f B);

/// @brief Lanewise square root.
GOLETA_FORCEINLINE Vec4f sqrt(Vec4f A);
/// @brief Lanewise reciprocal square root (hardware approximation, ~12-bit precision).
GOLETA_FORCEINLINE Vec4f rsqrt(Vec4f A);
/// @brief Lanewise reciprocal (hardware approximation, ~12-bit precision).
GOLETA_FORCEINLINE Vec4f rcp(Vec4f A);

/// @brief 3-lane dot product broadcast into every lane of the result.
GOLETA_FORCEINLINE Vec4f dot3(Vec4f A, Vec4f B);
/// @brief 4-lane dot product broadcast into every lane of the result.
GOLETA_FORCEINLINE Vec4f dot4(Vec4f A, Vec4f B);
/// @brief 3-lane cross product. Lane W of the result is 0.
GOLETA_FORCEINLINE Vec4f cross3(Vec4f A, Vec4f B);

/// @brief Lanewise `A == B` as an all-ones or zero mask.
GOLETA_FORCEINLINE Vec4f cmpEq(Vec4f A, Vec4f B);
/// @brief Lanewise `A != B` as an all-ones or zero mask.
GOLETA_FORCEINLINE Vec4f cmpNe(Vec4f A, Vec4f B);
/// @brief Lanewise `A < B` as an all-ones or zero mask.
GOLETA_FORCEINLINE Vec4f cmpLt(Vec4f A, Vec4f B);
/// @brief Lanewise `A <= B` as an all-ones or zero mask.
GOLETA_FORCEINLINE Vec4f cmpLe(Vec4f A, Vec4f B);
/// @brief Lanewise `A > B` as an all-ones or zero mask.
GOLETA_FORCEINLINE Vec4f cmpGt(Vec4f A, Vec4f B);
/// @brief Lanewise `A >= B` as an all-ones or zero mask.
GOLETA_FORCEINLINE Vec4f cmpGe(Vec4f A, Vec4f B);

/// @brief Lanewise bitwise AND on float bit patterns.
GOLETA_FORCEINLINE Vec4f andMask(Vec4f A, Vec4f B);
/// @brief Lanewise bitwise OR on float bit patterns.
GOLETA_FORCEINLINE Vec4f orMask(Vec4f A, Vec4f B);
/// @brief Lanewise bitwise XOR on float bit patterns.
GOLETA_FORCEINLINE Vec4f xorMask(Vec4f A, Vec4f B);
/// @brief Lanewise `~A & B` on float bit patterns.
GOLETA_FORCEINLINE Vec4f andNot(Vec4f A, Vec4f B);
/// @brief Lanewise select: returns A where mask is all-ones, B where zero.
GOLETA_FORCEINLINE Vec4f select(Vec4f Mask, Vec4f A, Vec4f B);

/// @brief Compile-time lane permutation: `result[i] = A[{X,Y,Z,W}[i]]`.
template <int X, int Y, int Z, int W>
GOLETA_FORCEINLINE Vec4f shuffle(Vec4f A);

/// @brief Compile-time two-source permutation: lanes {X,Y} from A, lanes {Z,W} from B.
template <int X, int Y, int Z, int W>
GOLETA_FORCEINLINE Vec4f shuffle2(Vec4f A, Vec4f B);

/// @brief Extracts lane 0 (X) as a scalar.
GOLETA_FORCEINLINE float getX(Vec4f A);
/// @brief Extracts lane 1 (Y) as a scalar.
GOLETA_FORCEINLINE float getY(Vec4f A);
/// @brief Extracts lane 2 (Z) as a scalar.
GOLETA_FORCEINLINE float getZ(Vec4f A);
/// @brief Extracts lane 3 (W) as a scalar.
GOLETA_FORCEINLINE float getW(Vec4f A);

/// @brief Broadcasts lane 0 (X) into every lane.
GOLETA_FORCEINLINE Vec4f splatX(Vec4f A);
/// @brief Broadcasts lane 1 (Y) into every lane.
GOLETA_FORCEINLINE Vec4f splatY(Vec4f A);
/// @brief Broadcasts lane 2 (Z) into every lane.
GOLETA_FORCEINLINE Vec4f splatZ(Vec4f A);
/// @brief Broadcasts lane 3 (W) into every lane.
GOLETA_FORCEINLINE Vec4f splatW(Vec4f A);

} // namespace goleta::simd

#if GOLETA_SIMD_SSE4
#include "Math/SIMD/Impl_SSE.inl"
#elif GOLETA_SIMD_NEON
#include "Math/SIMD/Impl_NEON.inl"
#else
#include "Math/SIMD/Impl_Scalar.inl"
#endif
