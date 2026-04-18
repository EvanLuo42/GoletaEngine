#pragma once

/// @file
/// @brief Portable compiler attribute macros: force-inline, alignment, etc.

#if defined(_MSC_VER)
#define GOLETA_FORCEINLINE __forceinline
#else
#define GOLETA_FORCEINLINE inline __attribute__((always_inline))
#endif

#define GOLETA_ALIGN16 alignas(16)
#define GOLETA_ALIGN32 alignas(32)
