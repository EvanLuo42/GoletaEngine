#pragma once

/// @file
/// @brief Device-global shader-visible descriptor heaps + free-list allocator.

#include <cstdint>
#include <mutex>
#include <vector>

#include "D3D12Prelude.h"
#include "RHIDescriptor.h"
#include "RHIHandle.h"

namespace goleta::rhi::d3d12
{

/// @brief One shader-visible ID3D12DescriptorHeap with a free-list slot allocator.
class ShaderVisibleHeap
{
public:
    bool initialize(ID3D12Device* Device, D3D12_DESCRIPTOR_HEAP_TYPE Type, uint32_t Capacity,
                    const char* DebugName) noexcept;

    /// @brief Reserve one slot. Returns InvalidBindlessIndex on exhaustion.
    uint32_t allocate() noexcept;

    /// @brief Reserve N *contiguous* slots. Returns InvalidBindlessIndex on exhaustion. Never
    ///        recycles from the free list; grows NextFresh only. Required for descriptor-set
    ///        layouts, which need contiguous ranges for root-table binds.
    uint32_t allocateRangeFresh(uint32_t Count) noexcept;

    /// @brief Return a slot to the pool. Safe with InvalidBindlessIndex (no-op).
    void free(uint32_t Index) noexcept;

    /// @brief CPU-side handle for the slot (for SRV/UAV/CBV/Sampler create calls).
    D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle(uint32_t Index) const noexcept;

    /// @brief GPU-side handle for the slot (for SetGraphicsRootDescriptorTable, etc.).
    D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle(uint32_t Index) const noexcept;

    ID3D12DescriptorHeap* heap() const noexcept { return Heap.Get(); }
    uint32_t              capacity() const noexcept { return Capacity_; }

private:
    ComPtr<ID3D12DescriptorHeap> Heap;
    D3D12_CPU_DESCRIPTOR_HANDLE  CpuStart{};
    D3D12_GPU_DESCRIPTOR_HANDLE  GpuStart{};
    uint32_t                     Increment = 0;
    uint32_t                     Capacity_ = 0;
    uint32_t                     NextFresh = 1; // Reserve 0 to keep InvalidBindlessIndex sentinel.
    std::vector<uint32_t>        FreeList;
    mutable std::mutex           Mutex;
};

/// @brief IRhiDescriptorHeap implementation; owns the CBV/SRV/UAV and Sampler shader-visible heaps.
class D3D12DescriptorHeap final : public IRhiDescriptorHeap
{
public:
    D3D12DescriptorHeap() noexcept = default;

    bool initialize(ID3D12Device* Device, uint32_t ResourceCapacity, uint32_t SamplerCapacity) noexcept;

    // IRhiResource
    RhiResourceKind kind() const noexcept override { return RhiResourceKind::DescriptorHeap; }
    const char*     debugName() const noexcept override { return Name.c_str(); }
    void            setDebugName(const char* NewName) override { Name = NewName ? NewName : ""; }

    // IRhiDescriptorHeap
    uint32_t capacity() const noexcept override { return Resource.capacity(); }
    uint32_t samplerCapacity() const noexcept override { return Sampler.capacity(); }

    ShaderVisibleHeap& resourceHeap() noexcept { return Resource; }
    ShaderVisibleHeap& samplerHeap() noexcept { return Sampler; }

private:
    ShaderVisibleHeap Resource;
    ShaderVisibleHeap Sampler;
    std::string       Name;
};

/// @brief Simple CPU-only descriptor heap used for RTV/DSV allocation.
class CpuDescriptorHeap
{
public:
    bool initialize(ID3D12Device* Device, D3D12_DESCRIPTOR_HEAP_TYPE Type, uint32_t Capacity) noexcept;

    /// @brief Returns InvalidBindlessIndex-like 0xFFFFFFFF on exhaustion.
    uint32_t allocate() noexcept;
    void     free(uint32_t Index) noexcept;

    D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle(uint32_t Index) const noexcept;

private:
    ComPtr<ID3D12DescriptorHeap> Heap;
    D3D12_CPU_DESCRIPTOR_HANDLE  CpuStart{};
    uint32_t                     Increment = 0;
    uint32_t                     Capacity_ = 0;
    uint32_t                     NextFresh = 0;
    std::vector<uint32_t>        FreeList;
    std::mutex                   Mutex;
};

} // namespace goleta::rhi::d3d12
