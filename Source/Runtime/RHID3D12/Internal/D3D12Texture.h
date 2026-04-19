#pragma once

/// @file
/// @brief IRhiTexture and IRhiTextureView over ID3D12Resource.

#include <string>
#include <vector>

#include "D3D12Prelude.h"
#include "RHITexture.h"

namespace goleta::rhi::d3d12
{

class D3D12Device;

/// @brief Shared storage for a texture. Separates resource lifetime from any views layered on top.
class D3D12Texture final : public IRhiTexture
{
public:
    static Rc<D3D12Texture> create(D3D12Device* Device, const RhiTextureDesc& Desc) noexcept;
    static Rc<D3D12Texture> createPlaced(D3D12Device* Device, ID3D12Heap* Heap, uint64_t Offset,
                                         const RhiTextureDesc& Desc) noexcept;
    /// @brief Adopt an existing ID3D12Resource (used for swap-chain back buffers).
    static Rc<D3D12Texture> wrapRaw(D3D12Device* Device, ComPtr<ID3D12Resource> Resource,
                                    const RhiTextureDesc& Desc) noexcept;

    ~D3D12Texture() override;

    static constexpr RhiResourceKind kExpectedKind = RhiResourceKind::Texture;

    // IRhiResource
    RhiResourceKind kind() const noexcept override { return kExpectedKind; }
    const char*     debugName() const noexcept override { return Name.c_str(); }
    void            setDebugName(const char* NewName) override;
    RhiNativeHandle nativeHandle() const noexcept override { return {RhiNativeHandleKind::D3D12Resource, Resource.Get()}; }

    // IRhiTexture
    const RhiTextureDesc& desc() const noexcept override { return Desc_; }
    RhiTextureHandle      srvHandle() const noexcept override { return {DefaultSrvIndex}; }
    RhiRwTextureHandle    uavHandle() const noexcept override { return {DefaultUavIndex}; }

    ID3D12Resource* raw() const noexcept { return Resource.Get(); }
    D3D12Device*    owner() const noexcept { return OwnerDevice; }

    // RTV / DSV descriptors are allocated lazily by the command list.
    uint32_t ensureRtvIndex(uint32_t MipSlice, uint32_t ArraySlice) noexcept;
    uint32_t ensureDsvIndex(uint32_t MipSlice, uint32_t ArraySlice) noexcept;

private:
    D3D12Texture() noexcept = default;

    void allocateDefaultBindlessViews() noexcept;

    D3D12Device*                 OwnerDevice = nullptr;
    ComPtr<ID3D12Resource>       Resource;
#if defined(GOLETA_RHID3D12_HAS_D3D12MA) && GOLETA_RHID3D12_HAS_D3D12MA
    ComPtr<D3D12MA::Allocation> Allocation;
#endif
    RhiTextureDesc Desc_{};
    uint32_t       DefaultSrvIndex = InvalidBindlessIndex;
    uint32_t       DefaultUavIndex = InvalidBindlessIndex;

    struct ViewCacheEntry
    {
        uint32_t Mip      = 0;
        uint32_t ArraySl  = 0;
        uint32_t HeapIdx  = 0xFFFFFFFFu;
    };
    std::vector<ViewCacheEntry> RtvCache;
    std::vector<ViewCacheEntry> DsvCache;
    std::string                 Name;
};

class D3D12TextureView final : public IRhiTextureView
{
public:
    static Rc<D3D12TextureView> create(D3D12Device* Device, const RhiTextureViewDesc& Desc) noexcept;
    ~D3D12TextureView() override;

    static constexpr RhiResourceKind kExpectedKind = RhiResourceKind::Texture;

    // IRhiResource
    RhiResourceKind kind() const noexcept override { return kExpectedKind; }
    const char*     debugName() const noexcept override { return Name.c_str(); }
    void            setDebugName(const char* NewName) override { Name = NewName ? NewName : ""; }

    // IRhiTextureView
    const RhiTextureViewDesc& desc() const noexcept override { return Desc_; }
    IRhiTexture*              texture() const noexcept override { return Owner.get(); }
    RhiTextureHandle          srvHandle() const noexcept override { return {SrvIndex}; }
    RhiRwTextureHandle        uavHandle() const noexcept override { return {UavIndex}; }

private:
    D3D12TextureView() noexcept = default;

    D3D12Device*       OwnerDevice = nullptr;
    Rc<IRhiTexture>    Owner;
    RhiTextureViewDesc Desc_{};
    uint32_t           SrvIndex = InvalidBindlessIndex;
    uint32_t           UavIndex = InvalidBindlessIndex;
    std::string        Name;
};

} // namespace goleta::rhi::d3d12
