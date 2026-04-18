#pragma once

/// @file
/// @brief SIMD ISA detection. Defines exactly one of GOLETA_SIMD_SSE4 /
///        GOLETA_SIMD_NEON / GOLETA_SIMD_SCALAR; GOLETA_SIMD_AVX2 is an
///        additive refinement over SSE4.

#if defined(__AVX2__)
#define GOLETA_SIMD_AVX2 1
#endif

#if defined(__SSE4_1__) || defined(_M_X64) || defined(__x86_64__)
#define GOLETA_SIMD_SSE4 1
#endif

#if defined(__ARM_NEON) || defined(__ARM_NEON__) || defined(_M_ARM64)
#define GOLETA_SIMD_NEON 1
#endif

#if !defined(GOLETA_SIMD_SSE4) && !defined(GOLETA_SIMD_NEON)
#define GOLETA_SIMD_SCALAR 1
#endif
