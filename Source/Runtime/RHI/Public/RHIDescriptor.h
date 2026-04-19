#pragma once

/// @file
/// @brief Descriptor API. The default path is a single global bindless heap; a thin
///        descriptor-set layout path is retained for emulation of set-based binding.

#include <cstdint>

#include "RHIEnums.h"
#include "RHIExport.h"
#include "RHIResource.h"
#include "RHIStructChain.h"

namespace goleta::rhi
{

class IRhiBuffer;
class IRhiTextureView;
class IRhiSampler;

/// @brief Kind of descriptor an entry in a traditional descriptor set holds.
enum class RhiDescriptorKind : uint8_t
{
    ConstantBuffer = 0,
    SampledTexture,
    StorageTexture,
    StorageBuffer,
    StructuredBuffer,
    Sampler,
    AccelStructure,
};

/// @brief One binding slot in a descriptor set layout.
struct RhiDescriptorBinding
{
    uint32_t           Binding       = 0;
    RhiDescriptorKind  Kind          = RhiDescriptorKind::ConstantBuffer;
    uint32_t           ArraySize     = 1;
    RhiShaderStageMask Visibility    = RhiShaderStageMask::All;
    bool               DynamicOffset = false;
};

/// @brief Layout of a traditional descriptor set. Prefer bindless unless you're emulating.
struct RhiDescriptorSetLayoutDesc
{
    static constexpr auto kStructType = RhiStructType::DescriptorSetLayoutDesc;
    RhiStructHeader       Header{kStructType, nullptr};

    const RhiDescriptorBinding* Bindings     = nullptr;
    uint32_t                    BindingCount = 0;

    const char* DebugName = nullptr;
};

class RHI_API IRhiDescriptorSetLayout : public IRhiResource
{
public:
    virtual const RhiDescriptorSetLayoutDesc& desc() const noexcept = 0;
};

/// @brief Allocated descriptor set. Writes land via updateXxx(); not thread-safe on a single set.
class RHI_API IRhiDescriptorSet : public IRhiResource
{
public:
    virtual IRhiDescriptorSetLayout* layout() const noexcept = 0;

    virtual void writeConstantBuffer(uint32_t Binding, uint32_t ArrayIndex, IRhiBuffer* Buffer, uint64_t Offset,
                                     uint64_t Size)                                                = 0;
    virtual void writeSampledTexture(uint32_t Binding, uint32_t ArrayIndex, IRhiTextureView* View) = 0;
    virtual void writeStorageTexture(uint32_t Binding, uint32_t ArrayIndex, IRhiTextureView* View) = 0;
    virtual void writeStorageBuffer(uint32_t Binding, uint32_t ArrayIndex, IRhiBuffer* Buffer, uint64_t Offset,
                                    uint64_t Size)                                                 = 0;
    virtual void writeSampler(uint32_t Binding, uint32_t ArrayIndex, IRhiSampler* Sampler)         = 0;
};

/// @brief The one, device-global bindless resource heap. Queried from the device — there is
///        only one per device. Every resource that exposes an SRV/UAV/sampler bindless index
///        has an entry here, allocated at resource creation time.
class RHI_API IRhiDescriptorHeap : public IRhiResource
{
public:
    virtual uint32_t capacity() const noexcept        = 0;
    virtual uint32_t samplerCapacity() const noexcept = 0;
};

struct RhiDescriptorHeapDesc
{
    static constexpr auto kStructType = RhiStructType::DescriptorHeapDesc;
    RhiStructHeader       Header{kStructType, nullptr};

    uint32_t ResourceCapacity = 1u << 20; // 1M descriptors default.
    uint32_t SamplerCapacity  = 2048;

    const char* DebugName = nullptr;
};

} // namespace goleta::rhi
