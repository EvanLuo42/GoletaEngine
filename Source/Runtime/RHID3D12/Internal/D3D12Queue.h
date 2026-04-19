#pragma once

/// @file
/// @brief D3D12 IRhiQueue implementation. Wraps ID3D12CommandQueue.

#include <string>

#include "D3D12Prelude.h"
#include "RHIQueue.h"

namespace goleta::rhi::d3d12
{

class D3D12Queue final : public IRhiQueue
{
public:
    static Rc<D3D12Queue> create(ID3D12Device* Device, RhiQueueKind Kind, const char* DebugName) noexcept;

    static constexpr RhiResourceKind kExpectedKind = RhiResourceKind::Queue;

    // IRhiResource
    RhiResourceKind kind() const noexcept override { return kExpectedKind; }
    const char*     debugName() const noexcept override { return Name.c_str(); }
    void            setDebugName(const char* NewName) override;
    RhiNativeHandle nativeHandle() const noexcept override { return {RhiNativeHandleKind::D3D12CommandQueue, Queue.Get()}; }

    // IRhiQueue
    RhiQueueKind          queueKind() const noexcept override { return Kind_; }
    Result<void, RhiError> submit(const RhiSubmitInfo& Submit) override;
    Result<void, RhiError> present(IRhiSwapChain* SwapChain, const RhiFenceWait* WaitFences,
                                   uint32_t WaitFenceCount) override;
    Result<void, RhiError> waitIdle() override;

    ID3D12CommandQueue* raw() const noexcept { return Queue.Get(); }

private:
    D3D12Queue() noexcept = default;

    ComPtr<ID3D12CommandQueue> Queue;
    RhiQueueKind               Kind_ = RhiQueueKind::Graphics;

    // Dedicated idle fence to implement waitIdle() without requiring caller fences.
    ComPtr<ID3D12Fence1>       IdleFence;
    HANDLE                     IdleEvent = nullptr;
    uint64_t                   IdleValue = 0;

    std::string Name;
};

D3D12_COMMAND_LIST_TYPE toD3dCommandListType(RhiQueueKind Kind) noexcept;

} // namespace goleta::rhi::d3d12
