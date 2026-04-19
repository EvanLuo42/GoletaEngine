#pragma once

/// @file
/// @brief IRhiQueryHeap (timestamp-only for MVP) over ID3D12QueryHeap.

#include <string>

#include "D3D12Prelude.h"
#include "RHIQuery.h"

namespace goleta::rhi::d3d12
{

class D3D12Device;

class D3D12QueryHeap final : public IRhiQueryHeap
{
public:
    static Rc<D3D12QueryHeap> create(D3D12Device* Device, const RhiQueryHeapDesc& Desc) noexcept;

    static constexpr RhiResourceKind kExpectedKind = RhiResourceKind::QueryHeap;

    // IRhiResource
    RhiResourceKind kind() const noexcept override { return kExpectedKind; }
    const char*     debugName() const noexcept override { return Name.c_str(); }
    void            setDebugName(const char* NewName) override;

    // IRhiQueryHeap
    const RhiQueryHeapDesc& desc() const noexcept override { return Desc_; }
    RhiQueryKind            queryKind() const noexcept override { return Desc_.Kind; }
    uint32_t                capacity() const noexcept override { return Desc_.Count; }

    ID3D12QueryHeap* raw() const noexcept { return Heap.Get(); }

    D3D12_QUERY_TYPE d3dType() const noexcept;

private:
    D3D12QueryHeap() noexcept = default;

    ComPtr<ID3D12QueryHeap> Heap;
    RhiQueryHeapDesc        Desc_{};
    std::string             Name;
};

} // namespace goleta::rhi::d3d12
