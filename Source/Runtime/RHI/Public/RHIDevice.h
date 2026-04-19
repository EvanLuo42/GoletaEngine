#pragma once

/// @file
/// @brief Device interface — the factory for every GPU-visible object.

#include <cstdint>

#include "Memory/Rc.h"
#include "RHIAdapter.h"
#include "RHIBuffer.h"
#include "RHIDescriptor.h"
#include "RHIEnums.h"
#include "RHIExport.h"
#include "RHIFeatures.h"
#include "RHIMemory.h"
#include "RHINativeHandle.h"
#include "RHIPipeline.h"
#include "RHIQuery.h"
#include "RHIResource.h"
#include "RHISampler.h"
#include "RHIShader.h"
#include "RHIStructChain.h"
#include "RHISwapChain.h"
#include "RHISync.h"
#include "RHITexture.h"
#include "Result.h"

namespace goleta::rhi
{

class IRhiCommandList;
class IRhiCommandPool;
class IRhiQueue;
class IRhiDebug;
class IRhiRayTracingPipeline;
class IRhiAccelStructure;
struct RhiRayTracingPipelineDesc;
struct RhiAccelStructureDesc;

struct RhiDeviceCreateInfo
{
    static constexpr auto kStructType = RhiStructType::DeviceCreateInfo;
    RhiStructHeader       Header{kStructType, nullptr};

    /// @brief Index into IRhiInstance::enumerateAdapters(). ~0u = pick first suitable.
    uint32_t AdapterIndex = ~uint32_t{0};

    /// @brief Enable optional features. Creation fails (ErrorUnsupported) if a requested
    ///        feature isn't available. Bits that are already mandatory at the core level
    ///        (timeline semaphores, enhanced barriers) are ignored.
    bool RequestRayTracing    = false;
    bool RequestMeshShading   = false;
    bool RequestWorkGraphs    = false;
    bool RequestHdrOutput     = false;
    bool RequestSparseBinding = false;

    const char* DebugName = nullptr;
};

/// @brief Top-level GPU device handle. Thread-safe for all creation methods.
class RHI_API IRhiDevice : public RefCounted
{
public:
    virtual const RhiAdapterInfo&    adapter() const noexcept  = 0;
    virtual const RhiDeviceFeatures& features() const noexcept = 0;
    virtual BackendKind              backend() const noexcept  = 0;

    virtual Rc<IRhiQueue> getQueue(RhiQueueKind Kind) = 0;

    virtual Rc<IRhiFence> createFence(uint64_t InitialValue = 0) = 0;

    virtual Rc<IRhiBuffer>       createBuffer(const RhiBufferDesc& Desc)             = 0;
    virtual Rc<IRhiTexture>      createTexture(const RhiTextureDesc& Desc)           = 0;
    virtual Rc<IRhiTextureView>  createTextureView(const RhiTextureViewDesc& Desc)   = 0;
    virtual Rc<IRhiSampler>      createSampler(const RhiSamplerDesc& Desc)           = 0;
    virtual Rc<IRhiShaderModule> createShaderModule(const RhiShaderModuleDesc& Desc) = 0;

    virtual Rc<IRhiMemoryHeap> createHeap(const RhiHeapDesc& Desc)                                                  = 0;
    virtual Rc<IRhiBuffer>     createPlacedBuffer(IRhiMemoryHeap* Heap, uint64_t Offset, const RhiBufferDesc& Desc) = 0;
    virtual Rc<IRhiTexture> createPlacedTexture(IRhiMemoryHeap* Heap, uint64_t Offset, const RhiTextureDesc& Desc)  = 0;

    virtual Rc<IRhiGraphicsPipeline> createGraphicsPipeline(const RhiGraphicsPipelineDesc& Desc) = 0;
    virtual Rc<IRhiComputePipeline>  createComputePipeline(const RhiComputePipelineDesc& Desc)   = 0;

    /// @brief Optional — returns null Rc when features().RayTracing.Supported is false.
    virtual Rc<IRhiRayTracingPipeline> createRayTracingPipeline(const RhiRayTracingPipelineDesc& Desc) = 0;
    virtual Rc<IRhiAccelStructure>     createAccelStructure(const RhiAccelStructureDesc& Desc)         = 0;

    virtual Rc<IRhiDescriptorSetLayout> createDescriptorSetLayout(const RhiDescriptorSetLayoutDesc& Desc) = 0;
    virtual Rc<IRhiDescriptorSet>       createDescriptorSet(IRhiDescriptorSetLayout* Layout)              = 0;
    virtual IRhiDescriptorHeap*         bindlessHeap() noexcept                                           = 0;

    virtual Rc<IRhiQueryHeap> createQueryHeap(const RhiQueryHeapDesc& Desc) = 0;

    /// @brief Create a per-thread command pool. Every command list must be allocated from a
    ///        pool; one pool per recording thread per queue kind. See IRhiCommandPool.
    virtual Rc<IRhiCommandPool> createCommandPool(RhiQueueKind Queue) = 0;

    virtual Rc<IRhiSwapChain> createSwapChain(const RhiSwapChainDesc& Desc) = 0;

    virtual Rc<IRhiDebug> debug() = 0;

    /// @brief Wait until every submitted command list on every queue has completed.
    /// @return Err(DeviceLost) if the device died during the wait.
    virtual Result<void, RhiError> waitIdle() = 0;

    /// @brief Native handle trap door; see Platform/RHIPlatform.h.
    virtual RhiNativeHandle nativeHandle() const noexcept { return RhiNativeHandle{}; }
};

} // namespace goleta::rhi
