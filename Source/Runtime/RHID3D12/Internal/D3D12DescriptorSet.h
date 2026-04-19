#pragma once

/// @file
/// @brief IRhiDescriptorSet + IRhiDescriptorSetLayout. Descriptor sets carve slot ranges out of
///        the shader-visible resource / sampler heaps.

#include <string>
#include <vector>

#include "D3D12Prelude.h"
#include "RHIDescriptor.h"
#include "RHIHandle.h"

namespace goleta::rhi::d3d12
{

class D3D12Device;

class D3D12DescriptorSetLayout final : public IRhiDescriptorSetLayout
{
public:
    static Rc<D3D12DescriptorSetLayout> create(const RhiDescriptorSetLayoutDesc& Desc) noexcept;

    static constexpr RhiResourceKind kExpectedKind = RhiResourceKind::DescriptorHeap;

    // IRhiResource
    RhiResourceKind kind() const noexcept override { return kExpectedKind; }
    const char*     debugName() const noexcept override { return Name.c_str(); }
    void            setDebugName(const char* NewName) override { Name = NewName ? NewName : ""; }

    // IRhiDescriptorSetLayout
    const RhiDescriptorSetLayoutDesc& desc() const noexcept override { return Desc_; }

    uint32_t resourceSlotCount() const noexcept { return ResourceSlots; }
    uint32_t samplerSlotCount() const noexcept { return SamplerSlots; }

    /// @brief Offset (in descriptor slots) of the given binding within the resource or sampler range.
    uint32_t resourceSlotOffset(uint32_t Binding) const noexcept;
    uint32_t samplerSlotOffset(uint32_t Binding) const noexcept;

private:
    D3D12DescriptorSetLayout() noexcept = default;

    RhiDescriptorSetLayoutDesc        Desc_{};
    std::vector<RhiDescriptorBinding> Bindings;

    std::vector<uint32_t> ResourceOffsets; // Index matches Bindings. 0xFFFFFFFF if sampler.
    std::vector<uint32_t> SamplerOffsets;

    uint32_t    ResourceSlots = 0;
    uint32_t    SamplerSlots  = 0;
    std::string Name;
};

class D3D12DescriptorSet final : public IRhiDescriptorSet
{
public:
    static Rc<D3D12DescriptorSet> create(D3D12Device* Device, IRhiDescriptorSetLayout* Layout) noexcept;

    ~D3D12DescriptorSet() override;

    static constexpr RhiResourceKind kExpectedKind = RhiResourceKind::DescriptorSet;

    // IRhiResource
    RhiResourceKind kind() const noexcept override { return kExpectedKind; }
    const char*     debugName() const noexcept override { return Name.c_str(); }
    void            setDebugName(const char* NewName) override { Name = NewName ? NewName : ""; }

    // IRhiDescriptorSet
    IRhiDescriptorSetLayout* layout() const noexcept override { return Layout_.get(); }

    void writeConstantBuffer(uint32_t Binding, uint32_t ArrayIndex, IRhiBuffer* Buffer, uint64_t Offset,
                             uint64_t Size) override;
    void writeSampledTexture(uint32_t Binding, uint32_t ArrayIndex, IRhiTextureView* View) override;
    void writeStorageTexture(uint32_t Binding, uint32_t ArrayIndex, IRhiTextureView* View) override;
    void writeStorageBuffer(uint32_t Binding, uint32_t ArrayIndex, IRhiBuffer* Buffer, uint64_t Offset,
                            uint64_t Size) override;
    void writeSampler(uint32_t Binding, uint32_t ArrayIndex, IRhiSampler* Sampler) override;

    uint32_t resourceBase() const noexcept { return ResourceBase; }
    uint32_t samplerBase() const noexcept { return SamplerBase; }

private:
    D3D12DescriptorSet() noexcept = default;

    D3D12Device*                OwnerDevice = nullptr;
    Rc<IRhiDescriptorSetLayout> Layout_;
    uint32_t                    ResourceBase  = InvalidBindlessIndex;
    uint32_t                    SamplerBase   = InvalidBindlessIndex;
    uint32_t                    ResourceCount = 0;
    uint32_t                    SamplerCount  = 0;
    std::string                 Name;
};

} // namespace goleta::rhi::d3d12
