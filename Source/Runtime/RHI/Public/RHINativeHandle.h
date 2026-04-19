#pragma once

/// @file
/// @brief Native-handle types: discriminator enum + discriminated pointer. Separated from
///        Platform/RHIPlatform.h so base interfaces can expose a virtual accessor without
///        pulling the platform trap-door API into every public header.

#include <cstdint>

namespace goleta::rhi
{

/// @brief Discriminator for the void* payload returned by a native-handle query.
enum class RhiNativeHandleKind : uint32_t
{
    Unknown = 0,

    D3D12Device       = 0x00010001,
    D3D12CommandQueue = 0x00010002,
    D3D12CommandList  = 0x00010003,
    D3D12Resource     = 0x00010004,
    D3D12Fence        = 0x00010005,

    VulkanInstance       = 0x00020001,
    VulkanPhysicalDevice = 0x00020002,
    VulkanDevice         = 0x00020003,
    VulkanQueue          = 0x00020004,
    VulkanCommandBuffer  = 0x00020005,
    VulkanBuffer         = 0x00020006,
    VulkanImage          = 0x00020007,
    VulkanSemaphore      = 0x00020008,

    PlatformPrivateBase = 0x80000000,
};

/// @brief Discriminated pointer into backend-internal state.
/// @note  The returned pointer is a non-owning view; its lifetime tracks the RHI object it was
///        queried from. Do not cast blindly — verify Kind first.
struct RhiNativeHandle
{
    RhiNativeHandleKind Kind   = RhiNativeHandleKind::Unknown;
    void*               Handle = nullptr;
};

} // namespace goleta::rhi
