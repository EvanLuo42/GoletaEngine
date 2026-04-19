#pragma once

/// @file
/// @brief IRhiDevice: owns the D3D12 device, memory allocator, queues, and descriptor heaps.

#include <array>
#include <mutex>
#include <string>

#include "D3D12DescriptorHeap.h"
#include "D3D12Prelude.h"
#include "RHIDebug.h"
#include "RHIDevice.h"
#include "RHIFeatures.h"

namespace goleta::rhi::d3d12
{

class D3D12Debug;
class D3D12Fence;
class D3D12Queue;

class D3D12Device final : public IRhiDevice
{
public:
    static Rc<D3D12Device> create(ComPtr<IDXGIFactory7> Factory, ComPtr<IDXGIAdapter4> Adapter,
                                  const RhiAdapterInfo& AdapterInfo, const RhiDeviceCreateInfo& Desc,
                                  RhiDebugLevel DebugLevel, RhiDebugCallback DebugCallback, void* DebugUser) noexcept;

    ~D3D12Device() override;

    // IRhiResource — not applicable (IRhiDevice does not inherit it).

    // IRhiDevice
    const RhiAdapterInfo&    adapter() const noexcept override { return Adapter_; }
    const RhiDeviceFeatures& features() const noexcept override { return Features_; }
    BackendKind              backend() const noexcept override { return BackendKind::D3D12; }

    Rc<IRhiQueue> getQueue(RhiQueueKind Kind) override;
    Rc<IRhiFence> createFence(uint64_t InitialValue = 0) override;

    Rc<IRhiBuffer>       createBuffer(const RhiBufferDesc& Desc) override;
    Rc<IRhiTexture>      createTexture(const RhiTextureDesc& Desc) override;
    Rc<IRhiTextureView>  createTextureView(const RhiTextureViewDesc& Desc) override;
    Rc<IRhiSampler>      createSampler(const RhiSamplerDesc& Desc) override;
    Rc<IRhiShaderModule> createShaderModule(const RhiShaderModuleDesc& Desc) override;

    Rc<IRhiMemoryHeap> createHeap(const RhiHeapDesc& Desc) override;
    Rc<IRhiBuffer>     createPlacedBuffer(IRhiMemoryHeap* Heap, uint64_t Offset, const RhiBufferDesc& Desc) override;
    Rc<IRhiTexture>    createPlacedTexture(IRhiMemoryHeap* Heap, uint64_t Offset, const RhiTextureDesc& Desc) override;

    Rc<IRhiGraphicsPipeline>   createGraphicsPipeline(const RhiGraphicsPipelineDesc& Desc) override;
    Rc<IRhiComputePipeline>    createComputePipeline(const RhiComputePipelineDesc& Desc) override;
    Rc<IRhiRayTracingPipeline> createRayTracingPipeline(const RhiRayTracingPipelineDesc& Desc) override;
    Rc<IRhiAccelStructure>     createAccelStructure(const RhiAccelStructureDesc& Desc) override;

    Rc<IRhiDescriptorSetLayout> createDescriptorSetLayout(const RhiDescriptorSetLayoutDesc& Desc) override;
    Rc<IRhiDescriptorSet>       createDescriptorSet(IRhiDescriptorSetLayout* Layout) override;
    IRhiDescriptorHeap*         bindlessHeap() noexcept override { return &BindlessHeap; }

    Rc<IRhiQueryHeap> createQueryHeap(const RhiQueryHeapDesc& Desc) override;

    Rc<IRhiCommandPool> createCommandPool(RhiQueueKind Queue) override;
    Rc<IRhiSwapChain>   createSwapChain(const RhiSwapChainDesc& Desc) override;

    Rc<IRhiDebug>          debug() override;
    Result<void, RhiError> waitIdle() override;
    RhiNativeHandle nativeHandle() const noexcept override { return {RhiNativeHandleKind::D3D12Device, Device.Get()}; }

    // Backend accessors
    ID3D12Device10*      raw() const noexcept { return Device.Get(); }
    IDXGIFactory7*       factory() const noexcept { return Factory.Get(); }
    IDXGIAdapter4*       dxgiAdapter() const noexcept { return Adapter.Get(); }
    D3D12DescriptorHeap& bindless() noexcept { return BindlessHeap; }
    CpuDescriptorHeap&   rtvHeap() noexcept { return Rtv; }
    CpuDescriptorHeap&   dsvHeap() noexcept { return Dsv; }

#if defined(GOLETA_RHID3D12_HAS_D3D12MA) && GOLETA_RHID3D12_HAS_D3D12MA
    D3D12MA::Allocator* allocator() const noexcept { return Allocator.Get(); }
#endif

private:
    D3D12Device() noexcept = default;

    void populateFeatures(const RhiDeviceCreateInfo& Desc) noexcept;

    ComPtr<IDXGIFactory7>  Factory;
    ComPtr<IDXGIAdapter4>  Adapter;
    ComPtr<ID3D12Device10> Device;

#if defined(GOLETA_RHID3D12_HAS_D3D12MA) && GOLETA_RHID3D12_HAS_D3D12MA
    ComPtr<D3D12MA::Allocator> Allocator;
#endif

    D3D12DescriptorHeap BindlessHeap;
    CpuDescriptorHeap   Rtv;
    CpuDescriptorHeap   Dsv;

    std::array<Rc<D3D12Queue>, static_cast<size_t>(RhiQueueKind::Count)> Queues;

    Rc<D3D12Debug>    DebugFacade;
    RhiAdapterInfo    Adapter_{};
    RhiDeviceFeatures Features_{};
    RhiDebugLevel     DebugLevel_ = RhiDebugLevel::None;
};

} // namespace goleta::rhi::d3d12
