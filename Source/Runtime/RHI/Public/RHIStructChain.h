#pragma once

/// @file
/// @brief pNext-style extension chain. Every create-info struct embeds a RhiStructHeader so
///        backends and vendor extensions can append optional data without ABI breaks.

#include <cstdint>

namespace goleta::rhi
{

/// @brief Identifier for every create-info / extension struct that participates in the chain.
/// @note  Values in [1, 0x7FFFFFFF] are reserved for the core RHI. Values in
///        [0x80000000, 0xFFFFFFFF] are reserved for backend / vendor extensions.
enum class RhiStructType : uint32_t
{
    Unknown = 0,

    InstanceCreateInfo      = 1,
    DeviceCreateInfo        = 2,
    BufferDesc              = 3,
    TextureDesc             = 4,
    SamplerDesc             = 5,
    ShaderModuleDesc        = 6,
    GraphicsPipelineDesc    = 7,
    ComputePipelineDesc     = 8,
    RayTracingPipelineDesc  = 9,
    AccelStructureDesc      = 10,
    HeapDesc                = 11,
    SwapChainDesc           = 12,
    DescriptorHeapDesc      = 13,
    DescriptorSetLayoutDesc = 14,
    QueryHeapDesc           = 15,
    RenderingDesc           = 16,
    BarrierGroup            = 17,
    SubmitInfo              = 18,
    DebugLayerSettings      = 19,

    BackendExtensionBase       = 0x80000000,
    GNMXExtShaderExport        = 0x80010001,
    D3D12XboxExtDirectStorage  = 0x80020001,
    NVNExtTextureBarrier       = 0x80030001,
    NvidiaExtCooperativeMatrix = 0x80040001,
};

/// @brief Common header embedded as the first member of every chain-participating struct.
struct RhiStructHeader
{
    RhiStructType sType = RhiStructType::Unknown;
    const void*   pNext = nullptr;
};

namespace detail
{
/// @brief Step the chain by one. Returns nullptr when End is reached.
inline const RhiStructHeader* nextChainLink(const RhiStructHeader* Head) noexcept
{
    if (!Head)
        return nullptr;
    return static_cast<const RhiStructHeader*>(Head->pNext);
}
} // namespace detail

/// @brief Locate the first chained struct with sType == Type, starting at Head.
/// @return Pointer to the matching struct (not cast to a specific type), or nullptr.
/// @note   O(N) in the chain length. Treats a null Head as an empty chain.
inline const RhiStructHeader* findChainLink(const RhiStructHeader* Head, const RhiStructType Type) noexcept
{
    for (const RhiStructHeader* Cursor = Head; Cursor != nullptr; Cursor = detail::nextChainLink(Cursor))
    {
        if (Cursor->sType == Type)
            return Cursor;
    }
    return nullptr;
}

/// @brief Locate the first chained struct of type T. T must begin with a RhiStructHeader and
///        must expose a static constexpr RhiStructType kStructType.
/// @return Typed pointer to the matching struct or nullptr.
template <class T>
inline const T* findChain(const RhiStructHeader* Head) noexcept
{
    const RhiStructHeader* Link = findChainLink(Head, T::kStructType);
    return static_cast<const T*>(static_cast<const void*>(Link));
}

} // namespace goleta::rhi
