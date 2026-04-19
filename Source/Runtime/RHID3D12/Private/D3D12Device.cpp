/// @file
/// @brief D3D12Device: owns ID3D12Device10, queues, descriptor heaps, allocator.

#include "D3D12Device.h"

#include "D3D12Buffer.h"
#include "D3D12CommandList.h"
#include "D3D12Debug.h"
#include "D3D12DescriptorSet.h"
#include "D3D12Fence.h"
#include "D3D12FormatTable.h"
#include "D3D12MemoryHeap.h"
#include "D3D12Pipeline.h"
#include "D3D12QueryHeap.h"
#include "D3D12Queue.h"
#include "D3D12Sampler.h"
#include "D3D12ShaderModule.h"
#include "D3D12SwapChain.h"
#include "D3D12Texture.h"

#include "RHIRayTracing.h"

namespace goleta::rhi::d3d12
{
namespace
{

RhiFormatCaps computeFormatCaps(ID3D12Device* Device, RhiFormat Format) noexcept
{
    const DXGI_FORMAT Dxgi = toDxgi(Format);
    if (Dxgi == DXGI_FORMAT_UNKNOWN)
        return RhiFormatCaps::None;
    D3D12_FEATURE_DATA_FORMAT_SUPPORT Data{Dxgi, D3D12_FORMAT_SUPPORT1_NONE, D3D12_FORMAT_SUPPORT2_NONE};
    if (FAILED(Device->CheckFeatureSupport(D3D12_FEATURE_FORMAT_SUPPORT, &Data, sizeof(Data))))
        return RhiFormatCaps::None;
    RhiFormatCaps Out = RhiFormatCaps::None;
    if (Data.Support1 & D3D12_FORMAT_SUPPORT1_SHADER_SAMPLE)        Out = Out | RhiFormatCaps::Sampled | RhiFormatCaps::SampledFiltered;
    if (Data.Support1 & D3D12_FORMAT_SUPPORT1_TYPED_UNORDERED_ACCESS_VIEW) Out = Out | RhiFormatCaps::Storage;
    if (Data.Support2 & D3D12_FORMAT_SUPPORT2_UAV_TYPED_LOAD)       Out = Out | RhiFormatCaps::Storage;
    if (Data.Support2 & D3D12_FORMAT_SUPPORT2_UAV_ATOMIC_ADD)       Out = Out | RhiFormatCaps::StorageAtomic;
    if (Data.Support1 & D3D12_FORMAT_SUPPORT1_RENDER_TARGET)        Out = Out | RhiFormatCaps::ColorAttachment;
    if (Data.Support1 & D3D12_FORMAT_SUPPORT1_BLENDABLE)            Out = Out | RhiFormatCaps::ColorBlend;
    if (Data.Support1 & D3D12_FORMAT_SUPPORT1_DEPTH_STENCIL)        Out = Out | RhiFormatCaps::DepthAttachment;
    if (Data.Support1 & D3D12_FORMAT_SUPPORT1_IA_VERTEX_BUFFER)     Out = Out | RhiFormatCaps::VertexAttribute;
    return Out;
}

// The feature struct exposes format caps as a plain function pointer with no user-data. To
// thread device context through that signature without risking cross-device aliasing, we
// precompute the per-format caps during device init into a process-global table keyed by a
// simple identity token.

constexpr size_t     kFormatCapsTableEntries = static_cast<size_t>(RhiFormat::Count);
RhiFormatCaps        g_FormatCapsTable[kFormatCapsTableEntries]{};

RhiFormatCaps formatCapsTrampoline(RhiFormat Format) noexcept
{
    const auto Idx = static_cast<size_t>(Format);
    return Idx < kFormatCapsTableEntries ? g_FormatCapsTable[Idx] : RhiFormatCaps::None;
}

} // namespace

Rc<D3D12Device> D3D12Device::create(ComPtr<IDXGIFactory7> Factory, ComPtr<IDXGIAdapter4> Adapter,
                                    const RhiAdapterInfo& AdapterInfo, const RhiDeviceCreateInfo& Desc,
                                    RhiDebugLevel DebugLevel, RhiDebugCallback DebugCallback,
                                    void* DebugUser) noexcept
{
    if (!Factory || !Adapter)
        return {};

    auto Self        = Rc<D3D12Device>(new D3D12Device{});
    Self->Factory    = std::move(Factory);
    Self->Adapter    = std::move(Adapter);
    Self->Adapter_   = AdapterInfo;
    Self->DebugLevel_ = DebugLevel;

    HRESULT Hr = D3D12CreateDevice(Self->Adapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&Self->Device));
    if (FAILED(Hr))
    {
        GOLETA_LOG_ERROR(D3D12, "D3D12CreateDevice: HRESULT 0x{:08x}", static_cast<unsigned>(Hr));
        return {};
    }

    // Enhanced-Barriers is mandatory for MVP. If the adapter lacks it we refuse creation with a
    // clear message; the Null backend remains available as a fallback.
    D3D12_FEATURE_DATA_D3D12_OPTIONS12 Options12{};
    Hr = Self->Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS12, &Options12, sizeof(Options12));
    if (FAILED(Hr) || !Options12.EnhancedBarriersSupported)
    {
        GOLETA_LOG_ERROR(D3D12, "Enhanced Barriers not supported by adapter '{}'; refusing device creation.",
                         AdapterInfo.Name);
        return {};
    }

#if defined(GOLETA_RHID3D12_HAS_D3D12MA) && GOLETA_RHID3D12_HAS_D3D12MA
    D3D12MA::ALLOCATOR_DESC AllocDesc{};
    AllocDesc.Flags    = D3D12MA::ALLOCATOR_FLAG_NONE;
    AllocDesc.pDevice  = Self->Device.Get();
    AllocDesc.pAdapter = Self->Adapter.Get();
    D3D12MA::Allocator* RawAlloc = nullptr;
    Hr = D3D12MA::CreateAllocator(&AllocDesc, &RawAlloc);
    if (FAILED(Hr))
    {
        GOLETA_LOG_WARN(D3D12, "D3D12MA::CreateAllocator failed (0x{:08x}); falling back to committed resources.",
                        static_cast<unsigned>(Hr));
    }
    else
    {
        Self->Allocator.Attach(RawAlloc);
    }
#endif

    // D3D12_MAX_SHADER_VISIBLE_DESCRIPTOR_HEAP_SIZE_TIER_2 caps CBV/SRV/UAV at 1,000,000, not
    // 1 << 20. Samplers cap at 2048.
    constexpr uint32_t kResourceHeapSize = 1'000'000u;
    constexpr uint32_t kSamplerHeapSize  = 2048;
    if (!Self->BindlessHeap.initialize(Self->Device.Get(), kResourceHeapSize, kSamplerHeapSize))
        return {};
    if (!Self->Rtv.initialize(Self->Device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 256))
        return {};
    if (!Self->Dsv.initialize(Self->Device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 64))
        return {};

    constexpr const char* kQueueNames[] = {"GfxQueue", "ComputeQueue", "CopyQueue", "VideoQueue"};
    for (uint32_t I = 0; I < static_cast<uint32_t>(RhiQueueKind::Count); ++I)
    {
        const auto K = static_cast<RhiQueueKind>(I);
        if (K == RhiQueueKind::Video)
            continue; // MVP: skip video queue to avoid platform-specific driver requirements.
        Self->Queues[I] = D3D12Queue::create(Self->Device.Get(), K, kQueueNames[I]);
    }

    Self->DebugFacade = D3D12Debug::create(Self->Device.Get(), DebugLevel, DebugCallback, DebugUser);

    for (size_t I = 0; I < kFormatCapsTableEntries; ++I)
        g_FormatCapsTable[I] = computeFormatCaps(Self->Device.Get(), static_cast<RhiFormat>(I));
    Self->populateFeatures(Desc);

    return Self;
}

D3D12Device::~D3D12Device() = default;

void D3D12Device::populateFeatures(const RhiDeviceCreateInfo&) noexcept
{
    Features_ = {};
    Features_.Core.TimelineSemaphore             = true;
    Features_.Core.EnhancedBarriers              = true;
    Features_.Core.DynamicRendering              = true;
    Features_.Core.ShaderModel66DynamicResources = true;
    Features_.Core.DescriptorBuffer              = false;

    Features_.BindlessResources.Supported         = true;
    Features_.BindlessResources.MaxSrvDescriptors = BindlessHeap.capacity();
    Features_.BindlessResources.MaxUavDescriptors = BindlessHeap.capacity();
    Features_.BindlessResources.MaxSamplers       = BindlessHeap.samplerCapacity();

    // Per-D3D12 limits.
    Features_.MaxTextureDimension1D   = D3D12_REQ_TEXTURE1D_U_DIMENSION;
    Features_.MaxTextureDimension2D   = D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION;
    Features_.MaxTextureDimension3D   = D3D12_REQ_TEXTURE3D_U_V_OR_W_DIMENSION;
    Features_.MaxTextureDimensionCube = D3D12_REQ_TEXTURECUBE_DIMENSION;
    Features_.MaxTextureArrayLayers   = D3D12_REQ_TEXTURE2D_ARRAY_AXIS_DIMENSION;
    Features_.MaxColorAttachments     = D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT;
    Features_.MaxViewports            = D3D12_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
    Features_.MaxComputeWorkgroupCountX = D3D12_CS_DISPATCH_MAX_THREAD_GROUPS_PER_DIMENSION;
    Features_.MaxComputeWorkgroupSizeX  = D3D12_CS_THREAD_GROUP_MAX_X;
    Features_.MaxPushConstantBytes      = 128;
    Features_.ConstantBufferAlignment   = D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT;
    Features_.StorageBufferAlignment    = 16;
    Features_.TexelBufferAlignment      = 16;
    Features_.NonCoherentAtomSize       = 256;

    Features_.FormatCapsFn = &formatCapsTrampoline;

    // Feature gates — MVP reports everything optional as unsupported.
    Features_.RayTracing.Supported    = false;
    Features_.MeshShading.Supported   = false;
    Features_.WorkGraphs.Supported    = false;
    Features_.SparseBinding.Supported = false;
    Features_.HdrOutput.Supported     = false;
    Features_.SamplerFeedback.Supported = false;
}

Rc<IRhiQueue> D3D12Device::getQueue(RhiQueueKind Kind)
{
    const auto Idx = static_cast<size_t>(Kind);
    if (Idx >= Queues.size())
        return {};
    return Queues[Idx];
}

Rc<IRhiFence> D3D12Device::createFence(uint64_t InitialValue)
{
    return D3D12Fence::create(Device.Get(), InitialValue);
}

Rc<IRhiBuffer> D3D12Device::createBuffer(const RhiBufferDesc& Desc)
{
    return D3D12Buffer::create(this, Desc);
}

Rc<IRhiTexture> D3D12Device::createTexture(const RhiTextureDesc& Desc)
{
    return D3D12Texture::create(this, Desc);
}

Rc<IRhiTextureView> D3D12Device::createTextureView(const RhiTextureViewDesc& Desc)
{
    return D3D12TextureView::create(this, Desc);
}

Rc<IRhiSampler> D3D12Device::createSampler(const RhiSamplerDesc& Desc)
{
    return D3D12Sampler::create(this, Desc);
}

Rc<IRhiShaderModule> D3D12Device::createShaderModule(const RhiShaderModuleDesc& Desc)
{
    return D3D12ShaderModule::create(Desc);
}

Rc<IRhiMemoryHeap> D3D12Device::createHeap(const RhiHeapDesc& Desc)
{
    return D3D12MemoryHeap::create(this, Desc);
}

Rc<IRhiBuffer> D3D12Device::createPlacedBuffer(IRhiMemoryHeap* Heap, uint64_t Offset, const RhiBufferDesc& Desc)
{
    auto* H = d3d12Cast<D3D12MemoryHeap>(Heap);
    return D3D12Buffer::createPlaced(this, H ? H->raw() : nullptr, Offset, Desc);
}

Rc<IRhiTexture> D3D12Device::createPlacedTexture(IRhiMemoryHeap* Heap, uint64_t Offset, const RhiTextureDesc& Desc)
{
    auto* H = d3d12Cast<D3D12MemoryHeap>(Heap);
    return D3D12Texture::createPlaced(this, H ? H->raw() : nullptr, Offset, Desc);
}

Rc<IRhiGraphicsPipeline> D3D12Device::createGraphicsPipeline(const RhiGraphicsPipelineDesc& Desc)
{
    return D3D12GraphicsPipeline::create(this, Desc);
}

Rc<IRhiComputePipeline> D3D12Device::createComputePipeline(const RhiComputePipelineDesc& Desc)
{
    return D3D12ComputePipeline::create(this, Desc);
}

Rc<IRhiRayTracingPipeline> D3D12Device::createRayTracingPipeline(const RhiRayTracingPipelineDesc&)
{
    // TODO(rhi): implement ray-tracing pipelines; see D3D12RayTracing.cpp.
    return {};
}

Rc<IRhiAccelStructure> D3D12Device::createAccelStructure(const RhiAccelStructureDesc&)
{
    // TODO(rhi): implement acceleration structures.
    return {};
}

Rc<IRhiDescriptorSetLayout> D3D12Device::createDescriptorSetLayout(const RhiDescriptorSetLayoutDesc& Desc)
{
    return D3D12DescriptorSetLayout::create(Desc);
}

Rc<IRhiDescriptorSet> D3D12Device::createDescriptorSet(IRhiDescriptorSetLayout* Layout)
{
    return D3D12DescriptorSet::create(this, Layout);
}

Rc<IRhiQueryHeap> D3D12Device::createQueryHeap(const RhiQueryHeapDesc& Desc)
{
    return D3D12QueryHeap::create(this, Desc);
}

Rc<IRhiCommandPool> D3D12Device::createCommandPool(RhiQueueKind Queue)
{
    return D3D12CommandPool::create(this, Queue);
}

Rc<IRhiSwapChain> D3D12Device::createSwapChain(const RhiSwapChainDesc& Desc)
{
    auto* Gfx = static_cast<D3D12Queue*>(Queues[static_cast<size_t>(RhiQueueKind::Graphics)].get());
    if (!Gfx || !Factory)
        return {};
    return D3D12SwapChain::create(this, Gfx, Factory.Get(), Desc);
}

Rc<IRhiDebug> D3D12Device::debug()
{
    return DebugFacade;
}

Result<void, RhiError> D3D12Device::waitIdle()
{
    for (auto& Q : Queues)
    {
        if (!Q) continue;
        auto R = Q->waitIdle();
        if (!R.isOk())
            return R;
    }
    return {};
}

} // namespace goleta::rhi::d3d12
