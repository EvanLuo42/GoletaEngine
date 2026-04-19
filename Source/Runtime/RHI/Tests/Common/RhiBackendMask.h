#pragma once

/// @file
/// @brief Compile-time bitmask over BackendKind used by GPU_TEST macro to gate which backends
///        a given test runs on. Mirrors slang-rhi's GpuTestFlags pattern.

#include <cstdint>

#include "RHIEnums.h"

namespace goleta::rhi::tests
{

/// @brief Bit-flag subset of BackendKind. Bit N = (1u << uint(BackendKind(N))).
enum class BackendMask : uint32_t
{
    None      = 0,
    Null      = 1u << static_cast<uint32_t>(BackendKind::Null),
    D3D12     = 1u << static_cast<uint32_t>(BackendKind::D3D12),
    Vulkan    = 1u << static_cast<uint32_t>(BackendKind::Vulkan),
    D3D12Xbox = 1u << static_cast<uint32_t>(BackendKind::D3D12Xbox),
    GNMX      = 1u << static_cast<uint32_t>(BackendKind::GNMX),
    NVN       = 1u << static_cast<uint32_t>(BackendKind::NVN),
    All       = Null | D3D12 | Vulkan | D3D12Xbox | GNMX | NVN,
};

constexpr BackendMask operator|(BackendMask A, BackendMask B) noexcept
{
    return static_cast<BackendMask>(static_cast<uint32_t>(A) | static_cast<uint32_t>(B));
}
constexpr BackendMask operator&(BackendMask A, BackendMask B) noexcept
{
    return static_cast<BackendMask>(static_cast<uint32_t>(A) & static_cast<uint32_t>(B));
}
constexpr BackendMask operator~(BackendMask A) noexcept
{
    return static_cast<BackendMask>(~static_cast<uint32_t>(A) & static_cast<uint32_t>(BackendMask::All));
}

constexpr bool contains(BackendMask Mask, BackendKind Kind) noexcept
{
    return (static_cast<uint32_t>(Mask) & (1u << static_cast<uint32_t>(Kind))) != 0;
}

/// @brief Convenience: everything except the Null (stub) backend. Use for tests that require
///        real GPU execution — reads/writes, dispatches, presents, etc.
inline constexpr BackendMask GpuOnly = BackendMask::All & ~BackendMask::Null;

/// @brief Stringify a BackendKind for readable test output. Pointer-stable static storage.
const char* backendName(BackendKind Kind) noexcept;

} // namespace goleta::rhi::tests
