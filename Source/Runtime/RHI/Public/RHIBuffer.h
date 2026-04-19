#pragma once

/// @file
/// @brief Buffer resource interface + creation descriptor.

#include <cstdint>

#include "RHIEnums.h"
#include "RHIExport.h"
#include "RHIHandle.h"
#include "RHIResource.h"
#include "RHIStructChain.h"

namespace goleta::rhi
{

/// @brief Creation parameters for a buffer.
struct RhiBufferDesc
{
    static constexpr auto kStructType = RhiStructType::BufferDesc;
    RhiStructHeader       Header{kStructType, nullptr};

    uint64_t          SizeBytes = 0;
    RhiBufferUsage    Usage     = RhiBufferUsage::None;
    RhiMemoryLocation Location  = RhiMemoryLocation::DeviceLocal;

    /// @brief Structured-buffer stride. Zero indicates a raw / byte-address buffer.
    uint32_t StructureStride = 0;

    const char* DebugName = nullptr;
};

/// @brief GPU buffer. Mapped only when Location is Upload or Readback.
class RHI_API IRhiBuffer : public IRhiResource
{
public:
    virtual const RhiBufferDesc& desc() const noexcept = 0;

    /// @brief Virtual GPU address. Zero if the device does not expose buffer addresses
    ///        (rare; all D3D12 and Vulkan 1.2+ devices support this).
    virtual uint64_t gpuAddress() const noexcept = 0;

    /// @brief Map a CPU-visible range. Returns nullptr for DeviceLocal buffers.
    /// @param Offset   Byte offset into the buffer.
    /// @param Size     Bytes to map. May be larger than the writable region the backend
    ///                 actually flushes — always bounded by desc().SizeBytes.
    virtual void* map(uint64_t Offset, uint64_t Size) = 0;

    /// @brief Release a mapping acquired via map(). Safe to call with no outstanding map.
    virtual void unmap() = 0;

    /// @brief Bindless shader-resource (read-only) index, if the buffer was created with
    ///        StorageBuffer / ConstantBuffer usage.
    /// @return InvalidBindlessIndex if the view kind is not applicable.
    virtual RhiBufferHandle srvHandle() const noexcept = 0;

    /// @brief Bindless unordered-access (read/write) index. Only valid when Usage has
    ///        StorageBuffer set.
    virtual RhiRwBufferHandle uavHandle() const noexcept = 0;
};

} // namespace goleta::rhi
