#pragma once

/// @file
/// @brief IRhiMemoryHeap: a raw ID3D12Heap used for placed-resource creation.

#include <string>

#include "D3D12Prelude.h"
#include "RHIMemory.h"

namespace goleta::rhi::d3d12
{

class D3D12Device;

class D3D12MemoryHeap final : public IRhiMemoryHeap
{
public:
    static Rc<D3D12MemoryHeap> create(D3D12Device* Device, const RhiHeapDesc& Desc) noexcept;

    static constexpr RhiResourceKind kExpectedKind = RhiResourceKind::MemoryHeap;

    // IRhiResource
    RhiResourceKind kind() const noexcept override { return kExpectedKind; }
    const char*     debugName() const noexcept override { return Name.c_str(); }
    void            setDebugName(const char* NewName) override;

    // IRhiMemoryHeap
    const RhiHeapDesc& desc() const noexcept override { return Desc_; }
    uint64_t           sizeBytes() const noexcept override { return Desc_.SizeBytes; }
    RhiMemoryLocation  location() const noexcept override { return Desc_.Location; }

    ID3D12Heap* raw() const noexcept { return Heap.Get(); }

private:
    D3D12MemoryHeap() noexcept = default;

    ComPtr<ID3D12Heap> Heap;
    RhiHeapDesc        Desc_{};
    std::string        Name;
};

} // namespace goleta::rhi::d3d12
