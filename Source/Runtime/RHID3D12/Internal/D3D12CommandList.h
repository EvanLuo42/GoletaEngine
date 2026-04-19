#pragma once

/// @file
/// @brief IRhiCommandList + IRhiCommandPool wrapping ID3D12GraphicsCommandList7 / ID3D12CommandAllocator.

#include <string>
#include <vector>

#include "D3D12Prelude.h"
#include "RHICommandList.h"

namespace goleta::rhi::d3d12
{

class D3D12Device;
class D3D12CommandPool;

class D3D12CommandList final : public IRhiCommandList
{
public:
    static Rc<D3D12CommandList> create(D3D12Device* Device, D3D12CommandPool* Pool, ID3D12CommandAllocator* Allocator,
                                       RhiQueueKind Kind) noexcept;

    static constexpr RhiResourceKind kExpectedKind = RhiResourceKind::CommandList;

    // IRhiResource
    RhiResourceKind kind() const noexcept override { return kExpectedKind; }
    const char*     debugName() const noexcept override { return Name.c_str(); }
    void            setDebugName(const char* NewName) override;
    RhiNativeHandle nativeHandle() const noexcept override
    {
        return {RhiNativeHandleKind::D3D12CommandList, List.Get()};
    }

    // IRhiCommandList — lifecycle
    RhiQueueKind queueKind() const noexcept override { return Kind_; }
    void         begin() override;
    void         end() override;
    void         reset() override;

    // Debug markers
    void beginDebugScope(const char* Name, uint32_t ColorRgba) override;
    void endDebugScope() override;
    void insertDebugMarker(const char* Name, uint32_t ColorRgba) override;

    // Barriers
    void barriers(const RhiBarrierGroup& Group) override;

    // Dynamic rendering
    void beginRendering(const RhiRenderingDesc& Desc) override;
    void endRendering() override;

    // State
    void setViewports(const RhiViewport* Viewports, uint32_t Count) override;
    void setScissors(const RhiRect* Scissors, uint32_t Count) override;
    void setBlendConstants(const float Rgba[4]) override;
    void setStencilReference(uint32_t Reference) override;

    void setGraphicsPipeline(IRhiGraphicsPipeline* Pipeline) override;
    void setComputePipeline(IRhiComputePipeline* Pipeline) override;
    void setRootConstants(const void* Data, uint32_t SizeBytes, uint32_t OffsetBytes) override;
    void setDescriptorSet(uint32_t SetIndex, IRhiDescriptorSet* Set) override;

    void setVertexBuffers(uint32_t FirstBinding, IRhiBuffer* const* Buffers, const uint64_t* Offsets,
                          uint32_t Count) override;
    void setIndexBuffer(IRhiBuffer* Buffer, uint64_t Offset, RhiIndexType Type) override;

    // Draw / dispatch
    void draw(uint32_t VertexCount, uint32_t InstanceCount, uint32_t FirstVertex, uint32_t FirstInstance) override;
    void drawIndexed(uint32_t IndexCount, uint32_t InstanceCount, uint32_t FirstIndex, int32_t VertexOffset,
                     uint32_t FirstInstance) override;
    void drawIndirect(IRhiBuffer* Args, uint64_t Offset, uint32_t DrawCount, uint32_t Stride) override;
    void drawIndexedIndirect(IRhiBuffer* Args, uint64_t Offset, uint32_t DrawCount, uint32_t Stride) override;
    void dispatch(uint32_t X, uint32_t Y, uint32_t Z) override;
    void dispatchIndirect(IRhiBuffer* Args, uint64_t Offset) override;

    // Copy
    void copyBuffer(IRhiBuffer* Dst, uint64_t DstOffset, IRhiBuffer* Src, uint64_t SrcOffset, uint64_t Size) override;
    void copyTexture(const RhiTextureCopy& Copy) override;
    void copyBufferToTexture(IRhiBuffer* Src, uint64_t SrcOffset, uint32_t RowPitch,
                             const RhiTextureCopy& Dst) override;

    // Queries
    void writeTimestamp(IRhiQueryHeap* Heap, uint32_t Index) override;
    void resolveQueries(IRhiQueryHeap* Heap, uint32_t FirstQuery, uint32_t Count, IRhiBuffer* Dst,
                        uint64_t DstOffset) override;

    // Optional features (MVP: unsupported).
    bool dispatchRays(const RhiDispatchRaysDesc& Desc) override;
    bool drawMeshTasks(uint32_t X, uint32_t Y, uint32_t Z) override;
    bool drawMeshTasksIndirect(IRhiBuffer* Args, uint64_t Offset, uint32_t DrawCount, uint32_t Stride) override;
    bool buildAccelStructure(const RhiAccelBuildDesc& Desc) override;

    ID3D12GraphicsCommandList7* raw() const noexcept { return List.Get(); }
    ID3D12CommandList*          rawList() const noexcept { return List.Get(); }

private:
    D3D12CommandList() noexcept = default;

    void bindDescriptorHeapsIfNeeded() noexcept;

    D3D12Device*                       OwnerDevice = nullptr;
    D3D12CommandPool*                  OwnerPool   = nullptr;
    ComPtr<ID3D12CommandAllocator>     Allocator;
    ComPtr<ID3D12GraphicsCommandList7> List;
    RhiQueueKind                       Kind_                = RhiQueueKind::Graphics;
    bool                               Recording            = false;
    bool                               InRendering          = false;
    bool                               HeapsBound           = false;
    bool                               GraphicsRootSigBound = false;
    bool                               ComputeRootSigBound  = false;
    std::string                        Name;
};

class D3D12CommandPool final : public IRhiCommandPool
{
public:
    static Rc<D3D12CommandPool> create(D3D12Device* Device, RhiQueueKind Kind) noexcept;

    static constexpr RhiResourceKind kExpectedKind = RhiResourceKind::CommandPool;

    // IRhiResource
    RhiResourceKind kind() const noexcept override { return kExpectedKind; }
    const char*     debugName() const noexcept override { return Name.c_str(); }
    void            setDebugName(const char* NewName) override;

    // IRhiCommandPool
    RhiQueueKind        queueKind() const noexcept override { return Kind_; }
    Rc<IRhiCommandList> allocate() override;
    void                reset() override;

    D3D12Device* owner() const noexcept { return OwnerDevice; }

private:
    D3D12CommandPool() noexcept = default;

    D3D12Device*                      OwnerDevice = nullptr;
    ComPtr<ID3D12CommandAllocator>    Allocator;
    RhiQueueKind                      Kind_ = RhiQueueKind::Graphics;
    std::string                       Name;
    std::vector<Rc<D3D12CommandList>> Lists;
};

} // namespace goleta::rhi::d3d12
