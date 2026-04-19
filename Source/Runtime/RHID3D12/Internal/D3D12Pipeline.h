#pragma once

/// @file
/// @brief ID3D12PipelineState wrappers for graphics and compute pipelines, plus root-signature
///        construction from RhiBindingLayout.

#include <string>

#include "D3D12Prelude.h"
#include "RHIPipeline.h"

namespace goleta::rhi::d3d12
{

class D3D12Device;

/// @brief Build a root signature matching the supplied binding layout. The result uses the
///        SM 6.6 CBV/SRV/UAV direct-heap-indexing model by default.
ComPtr<ID3D12RootSignature> buildRootSignature(ID3D12Device* Device, const RhiBindingLayout& Layout) noexcept;

class D3D12GraphicsPipeline final : public IRhiGraphicsPipeline
{
public:
    static Rc<D3D12GraphicsPipeline> create(D3D12Device* Device, const RhiGraphicsPipelineDesc& Desc) noexcept;

    static constexpr RhiResourceKind kExpectedKind = RhiResourceKind::GraphicsPipeline;

    // IRhiResource
    RhiResourceKind kind() const noexcept override { return kExpectedKind; }
    const char*     debugName() const noexcept override { return Name.c_str(); }
    void            setDebugName(const char* NewName) override;

    // IRhiGraphicsPipeline
    const RhiGraphicsPipelineDesc& desc() const noexcept override { return Desc_; }

    ID3D12PipelineState* raw() const noexcept { return State.Get(); }
    ID3D12RootSignature* rootSignature() const noexcept { return RootSig.Get(); }
    D3D12_PRIMITIVE_TOPOLOGY primitiveTopology() const noexcept { return Topology; }

private:
    D3D12GraphicsPipeline() noexcept = default;

    ComPtr<ID3D12PipelineState>  State;
    ComPtr<ID3D12RootSignature>  RootSig;
    RhiGraphicsPipelineDesc      Desc_{};
    D3D12_PRIMITIVE_TOPOLOGY     Topology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    std::string                  Name;
};

class D3D12ComputePipeline final : public IRhiComputePipeline
{
public:
    static Rc<D3D12ComputePipeline> create(D3D12Device* Device, const RhiComputePipelineDesc& Desc) noexcept;

    static constexpr RhiResourceKind kExpectedKind = RhiResourceKind::ComputePipeline;

    // IRhiResource
    RhiResourceKind kind() const noexcept override { return kExpectedKind; }
    const char*     debugName() const noexcept override { return Name.c_str(); }
    void            setDebugName(const char* NewName) override;

    // IRhiComputePipeline
    const RhiComputePipelineDesc& desc() const noexcept override { return Desc_; }

    ID3D12PipelineState* raw() const noexcept { return State.Get(); }
    ID3D12RootSignature* rootSignature() const noexcept { return RootSig.Get(); }

private:
    D3D12ComputePipeline() noexcept = default;

    ComPtr<ID3D12PipelineState> State;
    ComPtr<ID3D12RootSignature> RootSig;
    RhiComputePipelineDesc      Desc_{};
    std::string                 Name;
};

} // namespace goleta::rhi::d3d12
