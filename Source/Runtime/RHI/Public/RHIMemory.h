#pragma once

/// @file
/// @brief Explicit memory heaps + placed-resource API. The renderer owns suballocation policy.

#include <cstdint>

#include "RHIEnums.h"
#include "RHIExport.h"
#include "RHIResource.h"
#include "RHIStructChain.h"

namespace goleta::rhi
{

/// @brief Creation parameters for a memory heap.
struct RhiHeapDesc
{
    static constexpr auto kStructType = RhiStructType::HeapDesc;
    RhiStructHeader       Header{kStructType, nullptr};

    uint64_t          SizeBytes = 0;
    uint64_t          Alignment = 0; // 0 = backend default (64 KiB on D3D12).
    RhiMemoryLocation Location  = RhiMemoryLocation::DeviceLocal;

    /// @brief Hint that every resource placed here will be a buffer (enables tighter alignment
    ///        on tier-1 heap hardware). If false, heap must support all resource kinds.
    bool BufferOnly = false;

    const char* DebugName = nullptr;
};

/// @brief Opaque suballocatable heap owned by a device.
class RHI_API IRhiMemoryHeap : public IRhiResource
{
public:
    virtual const RhiHeapDesc& desc() const noexcept      = 0;
    virtual uint64_t           sizeBytes() const noexcept = 0;
    virtual RhiMemoryLocation  location() const noexcept  = 0;
};

} // namespace goleta::rhi
