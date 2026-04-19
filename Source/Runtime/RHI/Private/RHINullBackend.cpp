/// @file
/// @brief Built-in headless backend. Implements every interface with RAM-only stubs so the
///        RHI module is self-testable without a GPU or graphics SDK. Registers itself at
///        static-init via GOLETA_REGISTER_RHI_BACKEND(Null, ...).

#include <array>
#include <atomic>
#include <cstring>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

#include "RHIBackend.h"
#include "RHIBuffer.h"
#include "RHICommandList.h"
#include "RHIDebug.h"
#include "RHIDescriptor.h"
#include "RHIDevice.h"
#include "RHIInstance.h"
#include "RHIMemory.h"
#include "RHIPipeline.h"
#include "RHIQuery.h"
#include "RHIQueue.h"
#include "RHIRayTracing.h"
#include "RHISampler.h"
#include "RHIShader.h"
#include "RHISwapChain.h"
#include "RHISync.h"
#include "RHITexture.h"

namespace goleta::rhi
{
namespace
{

template <class Interface, RhiResourceKind KindValue>
class NullResourceBase : public Interface
{
public:
    [[nodiscard]] RhiResourceKind kind() const noexcept override { return KindValue; }
    [[nodiscard]] const char*     debugName() const noexcept override { return Name.c_str(); }
    void                          setDebugName(const char* NewName) override { Name = NewName ? NewName : ""; }

protected:
    explicit NullResourceBase(const char* DebugNameIn)
    {
        if (DebugNameIn)
            Name = DebugNameIn;
    }
    NullResourceBase() = default;

    std::string Name;
};

class NullFence final : public NullResourceBase<IRhiFence, RhiResourceKind::Fence>
{
public:
    explicit NullFence(const uint64_t Initial)
        : Value(Initial)
    {
    }

    uint64_t completedValue() const noexcept override { return Value.load(std::memory_order_acquire); }

    Result<RhiWaitStatus, RhiError> wait(const uint64_t Target, uint64_t /*TimeoutNanos*/) override
    {
        return Ok{completedValue() >= Target ? RhiWaitStatus::Reached : RhiWaitStatus::TimedOut};
    }

    Result<void, RhiError> signalFromCpu(const uint64_t NewValue) override
    {
        uint64_t Prev = Value.load(std::memory_order_relaxed);
        while (NewValue > Prev && !Value.compare_exchange_weak(Prev, NewValue, std::memory_order_acq_rel))
        {
        }
        return {};
    }

private:
    std::atomic<uint64_t> Value;
};

class NullBuffer final : public NullResourceBase<IRhiBuffer, RhiResourceKind::Buffer>
{
public:
    explicit NullBuffer(const RhiBufferDesc& DescIn)
        : NullResourceBase(DescIn.DebugName)
        , Desc_(DescIn)
        , Storage(DescIn.SizeBytes, std::byte{0})
    {
    }

    const RhiBufferDesc& desc() const noexcept override { return Desc_; }

    uint64_t gpuAddress() const noexcept override
    {
        return Storage.empty() ? 0 : reinterpret_cast<uint64_t>(Storage.data());
    }
    void* map(const uint64_t Offset, uint64_t /*Size*/) override
    {
        if (Desc_.Location == RhiMemoryLocation::DeviceLocal)
            return nullptr;
        if (Offset >= Storage.size())
            return nullptr;
        return Storage.data() + Offset;
    }
    void unmap() override {}

    RhiBufferHandle   srvHandle() const noexcept override { return {InvalidBindlessIndex}; }
    RhiRwBufferHandle uavHandle() const noexcept override { return {InvalidBindlessIndex}; }

private:
    RhiBufferDesc          Desc_;
    std::vector<std::byte> Storage;
};

class NullTexture final : public NullResourceBase<IRhiTexture, RhiResourceKind::Texture>
{
public:
    explicit NullTexture(const RhiTextureDesc& DescIn)
        : NullResourceBase(DescIn.DebugName)
        , Desc_(DescIn)
    {
    }

    const RhiTextureDesc& desc() const noexcept override { return Desc_; }

    RhiTextureHandle   srvHandle() const noexcept override { return {InvalidBindlessIndex}; }
    RhiRwTextureHandle uavHandle() const noexcept override { return {InvalidBindlessIndex}; }

private:
    RhiTextureDesc Desc_;
};

class NullTextureView final : public NullResourceBase<IRhiTextureView, RhiResourceKind::Texture>
{
public:
    explicit NullTextureView(const RhiTextureViewDesc& DescIn)
        : NullResourceBase(DescIn.DebugName)
        , Desc_(DescIn)
        , Owner(DescIn.Texture)
    {
    }

    const RhiTextureViewDesc& desc() const noexcept override { return Desc_; }
    IRhiTexture*              texture() const noexcept override { return Owner.get(); }

    RhiTextureHandle   srvHandle() const noexcept override { return {InvalidBindlessIndex}; }
    RhiRwTextureHandle uavHandle() const noexcept override { return {InvalidBindlessIndex}; }

private:
    RhiTextureViewDesc Desc_;
    Rc<IRhiTexture>    Owner;
};

class NullSampler final : public NullResourceBase<IRhiSampler, RhiResourceKind::Sampler>
{
public:
    explicit NullSampler(const RhiSamplerDesc& DescIn)
        : NullResourceBase(DescIn.DebugName)
        , Desc_(DescIn)
    {
    }

    const RhiSamplerDesc& desc() const noexcept override { return Desc_; }

    RhiSamplerHandle samplerHandle() const noexcept override { return {InvalidBindlessIndex}; }

private:
    RhiSamplerDesc Desc_;
};

class NullShaderModule final : public NullResourceBase<IRhiShaderModule, RhiResourceKind::ShaderModule>
{
public:
    explicit NullShaderModule(const RhiShaderModuleDesc& DescIn)
        : NullResourceBase(DescIn.DebugName)
        , Desc_(DescIn)
    {
        if (DescIn.Bytecode && DescIn.BytecodeSize > 0)
        {
            Blob.assign(static_cast<const std::byte*>(DescIn.Bytecode),
                        static_cast<const std::byte*>(DescIn.Bytecode) + DescIn.BytecodeSize);
            Desc_.Bytecode = Blob.data();
        }
        if (DescIn.EntryPoint)
            EntryPoint = DescIn.EntryPoint;
        Desc_.EntryPoint = EntryPoint.c_str();
    }

    const RhiShaderModuleDesc& desc() const noexcept override { return Desc_; }

    RhiShaderKind  shaderKind() const noexcept override { return Desc_.Kind; }
    RhiShaderStage stage() const noexcept override { return Desc_.Stage; }

private:
    RhiShaderModuleDesc    Desc_;
    std::vector<std::byte> Blob;
    std::string            EntryPoint;
};

class NullGraphicsPipeline final : public NullResourceBase<IRhiGraphicsPipeline, RhiResourceKind::GraphicsPipeline>
{
public:
    explicit NullGraphicsPipeline(const RhiGraphicsPipelineDesc& DescIn)
        : NullResourceBase(DescIn.DebugName)
        , Desc_(DescIn)
    {
    }
    const RhiGraphicsPipelineDesc& desc() const noexcept override { return Desc_; }

private:
    RhiGraphicsPipelineDesc Desc_;
};

class NullComputePipeline final : public NullResourceBase<IRhiComputePipeline, RhiResourceKind::ComputePipeline>
{
public:
    explicit NullComputePipeline(const RhiComputePipelineDesc& DescIn)
        : NullResourceBase(DescIn.DebugName)
        , Desc_(DescIn)
    {
    }
    const RhiComputePipelineDesc& desc() const noexcept override { return Desc_; }

private:
    RhiComputePipelineDesc Desc_;
};

class NullRayTracingPipeline final
    : public NullResourceBase<IRhiRayTracingPipeline, RhiResourceKind::RayTracingPipeline>
{
public:
    explicit NullRayTracingPipeline(const RhiRayTracingPipelineDesc& DescIn)
        : NullResourceBase(DescIn.DebugName)
        , Desc_(DescIn)
    {
    }
    const RhiRayTracingPipelineDesc& desc() const noexcept override { return Desc_; }

    uint32_t shaderGroupHandleSize() const noexcept override { return 32; }
    uint32_t shaderGroupHandleAlignment() const noexcept override { return 32; }

    void getShaderGroupHandles(uint32_t, uint32_t, void* OutBytes, uint32_t OutSize) override
    {
        if (OutBytes && OutSize > 0)
            std::memset(OutBytes, 0, OutSize);
    }

private:
    RhiRayTracingPipelineDesc Desc_;
};

class NullAccelStructure final : public NullResourceBase<IRhiAccelStructure, RhiResourceKind::AccelStructure>
{
public:
    explicit NullAccelStructure(const RhiAccelStructureDesc& DescIn)
        : NullResourceBase(DescIn.DebugName)
        , Desc_(DescIn)
    {
    }
    const RhiAccelStructureDesc& desc() const noexcept override { return Desc_; }

    uint64_t gpuAddress() const noexcept override { return 0; }
    uint64_t requiredScratchSize(bool) const noexcept override { return 0; }
    uint64_t resultBufferSize() const noexcept override { return 0; }

private:
    RhiAccelStructureDesc Desc_;
};

class NullDescriptorSetLayout final : public NullResourceBase<IRhiDescriptorSetLayout, RhiResourceKind::DescriptorHeap>
{
public:
    explicit NullDescriptorSetLayout(const RhiDescriptorSetLayoutDesc& DescIn)
        : NullResourceBase(DescIn.DebugName)
        , Desc_(DescIn)
    {
        if (DescIn.Bindings && DescIn.BindingCount > 0)
        {
            Bindings.assign(DescIn.Bindings, DescIn.Bindings + DescIn.BindingCount);
            Desc_.Bindings     = Bindings.data();
            Desc_.BindingCount = static_cast<uint32_t>(Bindings.size());
        }
    }
    const RhiDescriptorSetLayoutDesc& desc() const noexcept override { return Desc_; }

private:
    RhiDescriptorSetLayoutDesc        Desc_;
    std::vector<RhiDescriptorBinding> Bindings;
};

class NullDescriptorSet final : public NullResourceBase<IRhiDescriptorSet, RhiResourceKind::DescriptorSet>
{
public:
    explicit NullDescriptorSet(IRhiDescriptorSetLayout* LayoutIn)
        : Layout_(LayoutIn)
    {
    }
    IRhiDescriptorSetLayout* layout() const noexcept override { return Layout_.get(); }

    void writeConstantBuffer(uint32_t, uint32_t, IRhiBuffer*, uint64_t, uint64_t) override {}
    void writeSampledTexture(uint32_t, uint32_t, IRhiTextureView*) override {}
    void writeStorageTexture(uint32_t, uint32_t, IRhiTextureView*) override {}
    void writeStorageBuffer(uint32_t, uint32_t, IRhiBuffer*, uint64_t, uint64_t) override {}
    void writeSampler(uint32_t, uint32_t, IRhiSampler*) override {}

private:
    Rc<IRhiDescriptorSetLayout> Layout_;
};

class NullDescriptorHeap final : public NullResourceBase<IRhiDescriptorHeap, RhiResourceKind::DescriptorHeap>
{
public:
    uint32_t capacity() const noexcept override { return 1u << 20; }
    uint32_t samplerCapacity() const noexcept override { return 2048; }
};

class NullMemoryHeap final : public NullResourceBase<IRhiMemoryHeap, RhiResourceKind::MemoryHeap>
{
public:
    explicit NullMemoryHeap(const RhiHeapDesc& DescIn)
        : NullResourceBase(DescIn.DebugName)
        , Desc_(DescIn)
    {
    }
    const RhiHeapDesc& desc() const noexcept override { return Desc_; }
    uint64_t           sizeBytes() const noexcept override { return Desc_.SizeBytes; }
    RhiMemoryLocation  location() const noexcept override { return Desc_.Location; }

private:
    RhiHeapDesc Desc_;
};

class NullQueryHeap final : public NullResourceBase<IRhiQueryHeap, RhiResourceKind::QueryHeap>
{
public:
    explicit NullQueryHeap(const RhiQueryHeapDesc& DescIn)
        : NullResourceBase(DescIn.DebugName)
        , Desc_(DescIn)
    {
    }
    const RhiQueryHeapDesc& desc() const noexcept override { return Desc_; }
    RhiQueryKind            queryKind() const noexcept override { return Desc_.Kind; }
    uint32_t                capacity() const noexcept override { return Desc_.Count; }

private:
    RhiQueryHeapDesc Desc_;
};

class NullSwapChain final : public NullResourceBase<IRhiSwapChain, RhiResourceKind::SwapChain>
{
public:
    explicit NullSwapChain(const RhiSwapChainDesc& DescIn)
        : NullResourceBase(DescIn.DebugName)
        , Desc_(DescIn)
    {
        populateImages();
    }

    const RhiSwapChainDesc& desc() const noexcept override { return Desc_; }
    uint32_t                imageCount() const noexcept override { return Desc_.ImageCount; }
    RhiFormat               format() const noexcept override { return Desc_.Format; }
    uint32_t                width() const noexcept override { return Desc_.Width; }
    uint32_t                height() const noexcept override { return Desc_.Height; }
    IRhiTexture* image(uint32_t Index) override { return Index < Images.size() ? Images[Index].get() : nullptr; }
    uint32_t     currentImageIndex() const noexcept override { return CurrentImage; }
    Result<uint32_t, RhiError> acquireNextImage(const RhiFenceSignal* Signal) override
    {
        CurrentImage = Desc_.ImageCount == 0 ? 0 : (CurrentImage + 1u) % Desc_.ImageCount;
        if (Signal && Signal->Fence)
        {
            Signal->Fence->signalFromCpu(Signal->Value);
        }
        return Ok{CurrentImage};
    }
    void resize(uint32_t Width, uint32_t Height) override
    {
        Desc_.Width  = Width;
        Desc_.Height = Height;
        populateImages();
    }

private:
    void populateImages()
    {
        Images.clear();
        Images.reserve(Desc_.ImageCount);
        for (uint32_t I = 0; I < Desc_.ImageCount; ++I)
        {
            RhiTextureDesc Td{};
            Td.Width     = Desc_.Width;
            Td.Height    = Desc_.Height;
            Td.Format    = Desc_.Format;
            Td.Usage     = RhiTextureUsage::ColorAttachment | RhiTextureUsage::Sampled;
            Td.Dimension = RhiTextureDimension::Tex2D;
            Images.emplace_back(makeRc<NullTexture>(Td));
        }
    }

    RhiSwapChainDesc             Desc_;
    std::vector<Rc<IRhiTexture>> Images;
    uint32_t                     CurrentImage = 0;
};

class NullDebug final : public IRhiDebug
{
public:
    NullDebug(RhiDebugLevel Level, RhiDebugCallback Callback, void* User)
        : Level_(Level)
        , Callback_(Callback)
        , User_(User)
    {
    }

    RhiDebugLevel level() const noexcept override { return Level_; }

    void setMessageCallback(const RhiDebugCallback Callback, void* User) override
    {
        std::scoped_lock Lock(Mutex);
        Callback_ = Callback;
        User_     = User;
    }

    void pushMessageFilter(uint32_t, bool) override { ++FilterDepth_; }

    void popMessageFilter() override
    {
        if (FilterDepth_ > 0)
            --FilterDepth_;
    }

    void insertBreadcrumb(const char* Label) override
    {
        std::scoped_lock Lock(Mutex);
        LastBreadcrumb_ = Label ? Label : "";
    }

    bool tryGetLastCrashReport(RhiCrashReport&) override { return false; }

    void beginCapture(const char*) override {}

    void endCapture() override {}

    uint32_t filterDepth() const noexcept { return FilterDepth_; }

    std::string lastBreadcrumb()
    {
        std::scoped_lock Lock(Mutex);
        return LastBreadcrumb_;
    }

    void emit(const RhiDebugMessage& Message)
    {
        RhiDebugCallback Callback = nullptr;
        void*            User     = nullptr;
        {
            std::scoped_lock Lock(Mutex);
            Callback = Callback_;
            User     = User_;
        }
        if (Callback && Level_ != RhiDebugLevel::None)
            Callback(Message, User);
    }

private:
    RhiDebugLevel    Level_       = RhiDebugLevel::None;
    RhiDebugCallback Callback_    = nullptr;
    void*            User_        = nullptr;
    uint32_t         FilterDepth_ = 0;
    std::string      LastBreadcrumb_;
    std::mutex       Mutex;
};

class NullCommandList final : public NullResourceBase<IRhiCommandList, RhiResourceKind::CommandList>
{
public:
    explicit NullCommandList(const RhiQueueKind KindIn)
        : Kind_(KindIn)
    {
    }

    RhiQueueKind queueKind() const noexcept override { return Kind_; }

    void begin() override
    {
        Recording  = true;
        ScopeDepth = 0;
    }
    void end() override { Recording = false; }
    void reset() override
    {
        Recording  = false;
        ScopeDepth = 0;
    }

    void beginDebugScope(const char*, uint32_t) override { ++ScopeDepth; }
    void endDebugScope() override
    {
        if (ScopeDepth > 0)
            --ScopeDepth;
    }
    void insertDebugMarker(const char*, uint32_t) override {}

    void barriers(const RhiBarrierGroup&) override {}
    void beginRendering(const RhiRenderingDesc&) override { InRendering = true; }
    void endRendering() override { InRendering = false; }

    void setViewports(const RhiViewport*, uint32_t) override {}
    void setScissors(const RhiRect*, uint32_t) override {}
    void setBlendConstants(const float[4]) override {}
    void setStencilReference(uint32_t) override {}

    void setGraphicsPipeline(IRhiGraphicsPipeline*) override {}
    void setComputePipeline(IRhiComputePipeline*) override {}
    void setRootConstants(const void*, uint32_t, uint32_t) override {}
    void setDescriptorSet(uint32_t, IRhiDescriptorSet*) override {}

    void setVertexBuffers(uint32_t, IRhiBuffer* const*, const uint64_t*, uint32_t) override {}
    void setIndexBuffer(IRhiBuffer*, uint64_t, RhiIndexType) override {}

    void draw(uint32_t, uint32_t, uint32_t, uint32_t) override {}
    void drawIndexed(uint32_t, uint32_t, uint32_t, int32_t, uint32_t) override {}
    void drawIndirect(IRhiBuffer*, uint64_t, uint32_t, uint32_t) override {}
    void drawIndexedIndirect(IRhiBuffer*, uint64_t, uint32_t, uint32_t) override {}
    void dispatch(uint32_t, uint32_t, uint32_t) override {}
    void dispatchIndirect(IRhiBuffer*, uint64_t) override {}

    void copyBuffer(IRhiBuffer*, uint64_t, IRhiBuffer*, uint64_t, uint64_t) override {}
    void copyTexture(const RhiTextureCopy&) override {}
    void copyBufferToTexture(IRhiBuffer*, uint64_t, uint32_t, const RhiTextureCopy&) override {}

    void writeTimestamp(IRhiQueryHeap*, uint32_t) override {}
    void resolveQueries(IRhiQueryHeap*, uint32_t, uint32_t, IRhiBuffer*, uint64_t) override {}

    bool dispatchRays(const RhiDispatchRaysDesc&) override { return true; }
    bool drawMeshTasks(uint32_t, uint32_t, uint32_t) override { return true; }
    bool drawMeshTasksIndirect(IRhiBuffer*, uint64_t, uint32_t, uint32_t) override { return true; }
    bool buildAccelStructure(const RhiAccelBuildDesc&) override { return true; }

    bool     recording() const noexcept { return Recording; }
    uint32_t scopeDepth() const noexcept { return ScopeDepth; }

private:
    RhiQueueKind Kind_;
    bool         Recording   = false;
    bool         InRendering = false;
    uint32_t     ScopeDepth  = 0;
};

class NullCommandPool final : public NullResourceBase<IRhiCommandPool, RhiResourceKind::CommandPool>
{
public:
    explicit NullCommandPool(RhiQueueKind KindIn)
        : Kind_(KindIn)
    {
    }

    RhiQueueKind queueKind() const noexcept override { return Kind_; }

    Rc<IRhiCommandList> allocate() override
    {
        auto List = makeRc<NullCommandList>(Kind_);
        ++Allocated;
        return List;
    }

    void reset() override { Allocated = 0; }

    uint32_t allocatedCount() const noexcept { return Allocated; }

private:
    RhiQueueKind Kind_;
    uint32_t     Allocated = 0;
};

class NullQueue final : public NullResourceBase<IRhiQueue, RhiResourceKind::Queue>
{
public:
    explicit NullQueue(RhiQueueKind KindIn)
        : Kind_(KindIn)
    {
    }
    RhiQueueKind queueKind() const noexcept override { return Kind_; }

    Result<void, RhiError> submit(const RhiSubmitInfo& Info) override
    {
        for (uint32_t I = 0; I < Info.SignalFenceCount; ++I)
        {
            if (Info.SignalFences[I].Fence)
            {
                Info.SignalFences[I].Fence->signalFromCpu(Info.SignalFences[I].Value);
            }
        }
        return {};
    }
    Result<void, RhiError> present(IRhiSwapChain*, const RhiFenceWait*, uint32_t) override { return {}; }
    Result<void, RhiError> waitIdle() override { return {}; }

private:
    RhiQueueKind Kind_;
};

class NullDevice final : public IRhiDevice
{
public:
    NullDevice(const RhiAdapterInfo& AdapterIn, const RhiDeviceCreateInfo& Desc, Rc<NullDebug> DebugIn)
        : Adapter_(AdapterIn)
        , Debug_(std::move(DebugIn))
    {
        Features_.Core.TimelineSemaphore             = true;
        Features_.Core.EnhancedBarriers              = true;
        Features_.Core.DynamicRendering              = true;
        Features_.Core.ShaderModel66DynamicResources = true;

        Features_.BindlessResources.Supported         = true;
        Features_.BindlessResources.MaxSrvDescriptors = 1u << 20;
        Features_.BindlessResources.MaxUavDescriptors = 1u << 20;
        Features_.BindlessResources.MaxSamplers       = 2048;

        Features_.MaxTextureDimension1D   = 16384;
        Features_.MaxTextureDimension2D   = 16384;
        Features_.MaxTextureDimension3D   = 2048;
        Features_.MaxTextureDimensionCube = 16384;
        Features_.MaxTextureArrayLayers   = 2048;
        Features_.MaxColorAttachments     = 8;
        Features_.MaxViewports            = 16;
        Features_.MaxPushConstantBytes    = 128;
        Features_.ConstantBufferAlignment = 256;
        Features_.StorageBufferAlignment  = 16;
        Features_.TexelBufferAlignment    = 16;
        Features_.NonCoherentAtomSize     = 64;

        Features_.RayTracing.Supported            = Desc.RequestRayTracing;
        Features_.RayTracing.MaxRayRecursionDepth = Desc.RequestRayTracing ? 31u : 0u;
        Features_.RayTracing.RayQuery             = Desc.RequestRayTracing;
        Features_.RayTracing.InlineRayTracing     = Desc.RequestRayTracing;
        Features_.MeshShading.Supported           = Desc.RequestMeshShading;
        Features_.WorkGraphs.Supported            = Desc.RequestWorkGraphs;
        Features_.HdrOutput.Supported             = Desc.RequestHdrOutput;
        Features_.SparseBinding.Supported         = Desc.RequestSparseBinding;

        for (size_t I = 0; I < Queues_.size(); ++I)
        {
            Queues_[I] = makeRc<NullQueue>(static_cast<RhiQueueKind>(I));
        }
        BindlessHeap_ = makeRc<NullDescriptorHeap>();
    }

    const RhiAdapterInfo&    adapter() const noexcept override { return Adapter_; }
    const RhiDeviceFeatures& features() const noexcept override { return Features_; }
    BackendKind              backend() const noexcept override { return BackendKind::Null; }

    Rc<IRhiQueue> getQueue(RhiQueueKind QueueKind) override
    {
        const size_t I = static_cast<size_t>(QueueKind);
        if (I >= Queues_.size())
            return Rc<IRhiQueue>{};
        return Queues_[I];
    }

    Rc<IRhiFence> createFence(uint64_t Initial = 0) override { return makeRc<NullFence>(Initial); }

    Rc<IRhiBuffer> createBuffer(const RhiBufferDesc& Desc) override { return makeRc<NullBuffer>(Desc); }

    Rc<IRhiTexture>     createTexture(const RhiTextureDesc& Desc) override { return makeRc<NullTexture>(Desc); }
    Rc<IRhiTextureView> createTextureView(const RhiTextureViewDesc& Desc) override
    {
        return makeRc<NullTextureView>(Desc);
    }

    Rc<IRhiSampler> createSampler(const RhiSamplerDesc& Desc) override { return makeRc<NullSampler>(Desc); }

    Rc<IRhiShaderModule> createShaderModule(const RhiShaderModuleDesc& Desc) override
    {
        return makeRc<NullShaderModule>(Desc);
    }

    Rc<IRhiMemoryHeap> createHeap(const RhiHeapDesc& Desc) override { return makeRc<NullMemoryHeap>(Desc); }
    Rc<IRhiBuffer>     createPlacedBuffer(IRhiMemoryHeap*, uint64_t, const RhiBufferDesc& Desc) override
    {
        return makeRc<NullBuffer>(Desc);
    }
    Rc<IRhiTexture> createPlacedTexture(IRhiMemoryHeap*, uint64_t, const RhiTextureDesc& Desc) override
    {
        return makeRc<NullTexture>(Desc);
    }

    Rc<IRhiGraphicsPipeline> createGraphicsPipeline(const RhiGraphicsPipelineDesc& Desc) override
    {
        return makeRc<NullGraphicsPipeline>(Desc);
    }
    Rc<IRhiComputePipeline> createComputePipeline(const RhiComputePipelineDesc& Desc) override
    {
        return makeRc<NullComputePipeline>(Desc);
    }

    Rc<IRhiRayTracingPipeline> createRayTracingPipeline(const RhiRayTracingPipelineDesc& Desc) override
    {
        if (!Features_.RayTracing.Supported)
            return Rc<IRhiRayTracingPipeline>{};
        return makeRc<NullRayTracingPipeline>(Desc);
    }
    Rc<IRhiAccelStructure> createAccelStructure(const RhiAccelStructureDesc& Desc) override
    {
        if (!Features_.RayTracing.Supported)
            return Rc<IRhiAccelStructure>{};
        return makeRc<NullAccelStructure>(Desc);
    }

    Rc<IRhiDescriptorSetLayout> createDescriptorSetLayout(const RhiDescriptorSetLayoutDesc& Desc) override
    {
        return makeRc<NullDescriptorSetLayout>(Desc);
    }
    Rc<IRhiDescriptorSet> createDescriptorSet(IRhiDescriptorSetLayout* Layout) override
    {
        return makeRc<NullDescriptorSet>(Layout);
    }
    IRhiDescriptorHeap* bindlessHeap() noexcept override { return BindlessHeap_.get(); }

    Rc<IRhiQueryHeap>   createQueryHeap(const RhiQueryHeapDesc& Desc) override { return makeRc<NullQueryHeap>(Desc); }
    Rc<IRhiCommandPool> createCommandPool(RhiQueueKind Queue) override { return makeRc<NullCommandPool>(Queue); }
    Rc<IRhiSwapChain>   createSwapChain(const RhiSwapChainDesc& Desc) override { return makeRc<NullSwapChain>(Desc); }

    Rc<IRhiDebug> debug() override { return Rc<IRhiDebug>(Debug_.get()); }

    Result<void, RhiError> waitIdle() override { return {}; }

private:
    RhiAdapterInfo                                                      Adapter_;
    RhiDeviceFeatures                                                   Features_{};
    std::array<Rc<IRhiQueue>, static_cast<size_t>(RhiQueueKind::Count)> Queues_;
    Rc<IRhiDescriptorHeap>                                              BindlessHeap_;
    Rc<NullDebug>                                                       Debug_;
};

class NullInstance final : public IRhiInstance
{
public:
    explicit NullInstance(const RhiInstanceCreateInfo& Desc)
        : Debug_(makeRc<NullDebug>(Desc.DebugLevel, Desc.MessageCallback, Desc.MessageCallbackUser))
    {
    }

    BackendKind backend() const noexcept override { return BackendKind::Null; }

    std::vector<RhiAdapterInfo> enumerateAdapters() const override
    {
        RhiAdapterInfo Adapter{};
        const char*    Name = "Goleta Null Backend";
        std::strncpy(Adapter.Name, Name, sizeof(Adapter.Name) - 1);
        Adapter.VendorId = 0;
        Adapter.DeviceId = 0;
        Adapter.Kind     = RhiAdapterKind::Software;
        Adapter.Backend  = BackendKind::Null;
        return {Adapter};
    }

    Rc<IRhiDevice> createDevice(const RhiDeviceCreateInfo& Desc) override
    {
        auto           Adapters = enumerateAdapters();
        const uint32_t Index    = Desc.AdapterIndex == ~uint32_t{0} ? 0u : Desc.AdapterIndex;
        if (Index >= Adapters.size())
            return Rc<IRhiDevice>{};
        return makeRc<NullDevice>(Adapters[Index], Desc, Debug_);
    }

private:
    Rc<NullDebug> Debug_;
};

Rc<IRhiInstance> createNullInstance(const RhiInstanceCreateInfo& Desc) { return makeRc<NullInstance>(Desc); }

} // namespace
} // namespace goleta::rhi

GOLETA_REGISTER_RHI_BACKEND(Null, &::goleta::rhi::createNullInstance)
