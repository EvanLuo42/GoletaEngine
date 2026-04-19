#pragma once

/// @file
/// @brief IRhiBuffer implementation over ID3D12Resource + D3D12MA::Allocation.

#include <atomic>
#include <string>

#include "D3D12Prelude.h"
#include "RHIBuffer.h"

namespace goleta::rhi::d3d12
{

class D3D12Device;

class D3D12Buffer final : public IRhiBuffer
{
public:
    static Rc<D3D12Buffer> create(D3D12Device* Device, const RhiBufferDesc& Desc) noexcept;
    static Rc<D3D12Buffer> createPlaced(D3D12Device* Device, ID3D12Heap* Heap, uint64_t Offset,
                                        const RhiBufferDesc& Desc) noexcept;
    static Rc<D3D12Buffer> wrapRaw(D3D12Device* Device, ComPtr<ID3D12Resource> Resource,
                                   const RhiBufferDesc& Desc) noexcept;

    ~D3D12Buffer() override;

    static constexpr RhiResourceKind kExpectedKind = RhiResourceKind::Buffer;

    // IRhiResource
    RhiResourceKind kind() const noexcept override { return kExpectedKind; }
    const char*     debugName() const noexcept override { return Name.c_str(); }
    void            setDebugName(const char* NewName) override;
    RhiNativeHandle nativeHandle() const noexcept override
    {
        return {RhiNativeHandleKind::D3D12Resource, Resource.Get()};
    }

    // IRhiBuffer
    const RhiBufferDesc& desc() const noexcept override { return Desc_; }
    uint64_t          gpuAddress() const noexcept override { return Resource ? Resource->GetGPUVirtualAddress() : 0; }
    void*             map(uint64_t Offset, uint64_t Size) override;
    void              unmap() override;
    RhiBufferHandle   srvHandle() const noexcept override { return {SrvIndex}; }
    RhiRwBufferHandle uavHandle() const noexcept override { return {UavIndex}; }

    ID3D12Resource* raw() const noexcept { return Resource.Get(); }

private:
    D3D12Buffer() noexcept = default;

    void allocateBindlessViews() noexcept;

    D3D12Device*           OwnerDevice = nullptr;
    ComPtr<ID3D12Resource> Resource;
#if defined(GOLETA_RHID3D12_HAS_D3D12MA) && GOLETA_RHID3D12_HAS_D3D12MA
    ComPtr<D3D12MA::Allocation> Allocation;
#endif
    RhiBufferDesc Desc_{};
    uint32_t      SrvIndex  = InvalidBindlessIndex;
    uint32_t      UavIndex  = InvalidBindlessIndex;
    void*         MappedPtr = nullptr;
    std::string   Name;
};

} // namespace goleta::rhi::d3d12
