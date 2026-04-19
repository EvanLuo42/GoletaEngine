#pragma once

/// @file
/// @brief IRhiSampler implementation; allocates one slot in the shader-visible sampler heap.

#include <string>

#include "D3D12Prelude.h"
#include "RHISampler.h"

namespace goleta::rhi::d3d12
{

class D3D12Device;

class D3D12Sampler final : public IRhiSampler
{
public:
    static Rc<D3D12Sampler> create(D3D12Device* Device, const RhiSamplerDesc& Desc) noexcept;

    ~D3D12Sampler() override;

    static constexpr RhiResourceKind kExpectedKind = RhiResourceKind::Sampler;

    // IRhiResource
    RhiResourceKind kind() const noexcept override { return kExpectedKind; }
    const char*     debugName() const noexcept override { return Name.c_str(); }
    void            setDebugName(const char* NewName) override { Name = NewName ? NewName : ""; }

    // IRhiSampler
    const RhiSamplerDesc& desc() const noexcept override { return Desc_; }
    RhiSamplerHandle      samplerHandle() const noexcept override { return {Index}; }

    const D3D12_SAMPLER_DESC& d3dDesc() const noexcept { return D3dDesc; }

private:
    D3D12Sampler() noexcept = default;

    D3D12Device*       OwnerDevice = nullptr;
    RhiSamplerDesc     Desc_{};
    D3D12_SAMPLER_DESC D3dDesc{};
    uint32_t           Index = InvalidBindlessIndex;
    std::string        Name;
};

} // namespace goleta::rhi::d3d12
