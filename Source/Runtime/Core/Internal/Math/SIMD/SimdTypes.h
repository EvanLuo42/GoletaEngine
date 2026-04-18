#pragma once

/// @file
/// @brief Backend-specific aliases for the 4-lane SIMD vector types.

#include <cstdint>

#include "Compiler.h"
#include "Math/SIMD/SimdConfig.h"

#if GOLETA_SIMD_SSE4
#include <immintrin.h>
#elif GOLETA_SIMD_NEON
#include <arm_neon.h>
#endif

namespace goleta::simd
{

#if GOLETA_SIMD_SSE4

using Vec4f = __m128;
using Vec4i = __m128i;

#elif GOLETA_SIMD_NEON

using Vec4f = float32x4_t;
using Vec4i = int32x4_t;

#else

struct GOLETA_ALIGN16 Vec4f
{
    float V[4];
};
struct GOLETA_ALIGN16 Vec4i
{
    int32_t V[4];
};

#endif

} // namespace goleta::simd
