/// @file
/// @brief ID3D12CommandQueue wrapper.

#include "D3D12Queue.h"

#include "D3D12CommandList.h"
#include "D3D12Fence.h"
#include "D3D12SwapChain.h"

namespace goleta::rhi::d3d12
{

D3D12_COMMAND_LIST_TYPE toD3dCommandListType(RhiQueueKind Kind) noexcept
{
    switch (Kind)
    {
    case RhiQueueKind::Graphics: return D3D12_COMMAND_LIST_TYPE_DIRECT;
    case RhiQueueKind::Compute:  return D3D12_COMMAND_LIST_TYPE_COMPUTE;
    case RhiQueueKind::Copy:     return D3D12_COMMAND_LIST_TYPE_COPY;
    case RhiQueueKind::Video:    return D3D12_COMMAND_LIST_TYPE_VIDEO_DECODE;
    default:                     return D3D12_COMMAND_LIST_TYPE_DIRECT;
    }
}

Rc<D3D12Queue> D3D12Queue::create(ID3D12Device* Device, RhiQueueKind Kind, const char* DebugName) noexcept
{
    if (!Device)
        return {};

    auto Self = Rc<D3D12Queue>(new D3D12Queue{});
    Self->Kind_ = Kind;

    D3D12_COMMAND_QUEUE_DESC Desc{};
    Desc.Type     = toD3dCommandListType(Kind);
    Desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
    Desc.Flags    = D3D12_COMMAND_QUEUE_FLAG_NONE;
    Desc.NodeMask = 0;
    HRESULT Hr = Device->CreateCommandQueue(&Desc, IID_PPV_ARGS(&Self->Queue));
    if (FAILED(Hr))
    {
        GOLETA_LOG_ERROR(D3D12, "CreateCommandQueue(kind={}): HRESULT 0x{:08x}", static_cast<int>(Kind),
                         static_cast<unsigned>(Hr));
        return {};
    }
    if (DebugName)
        setD3dObjectName(Self->Queue.Get(), DebugName);

    Hr = Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&Self->IdleFence));
    if (FAILED(Hr))
    {
        GOLETA_LOG_ERROR(D3D12, "Queue idle-fence CreateFence: HRESULT 0x{:08x}", static_cast<unsigned>(Hr));
        return {};
    }
    Self->IdleEvent = ::CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS);
    if (!Self->IdleEvent)
        return {};

    return Self;
}

void D3D12Queue::setDebugName(const char* NewName)
{
    Name = NewName ? NewName : "";
    if (Queue)
        setD3dObjectName(Queue.Get(), Name.c_str());
}

Result<void, RhiError> D3D12Queue::submit(const RhiSubmitInfo& Submit)
{
    if (!Queue)
        return Err{RhiError::Unknown};

    // Wait phase.
    for (uint32_t I = 0; I < Submit.WaitFenceCount; ++I)
    {
        auto* Fence = d3d12Cast<D3D12Fence>(Submit.WaitFences[I].Fence);
        if (!Fence)
            continue;
        const HRESULT Hr = Queue->Wait(Fence->raw(), Submit.WaitFences[I].Value);
        if (FAILED(Hr))
        {
            GOLETA_LOG_ERROR(D3D12, "Queue::Wait: HRESULT 0x{:08x}", static_cast<unsigned>(Hr));
            return Err{hresultToRhiError(Hr)};
        }
    }

    // Execute phase.
    if (Submit.CommandListCount > 0 && Submit.CommandLists)
    {
        constexpr uint32_t kMaxInline = 16;
        ID3D12CommandList* Inline[kMaxInline];
        std::vector<ID3D12CommandList*> Heap;
        ID3D12CommandList** Ptr = Inline;
        if (Submit.CommandListCount > kMaxInline)
        {
            Heap.resize(Submit.CommandListCount);
            Ptr = Heap.data();
        }
        for (uint32_t I = 0; I < Submit.CommandListCount; ++I)
        {
            auto* Cl  = d3d12Cast<D3D12CommandList>(Submit.CommandLists[I]);
            Ptr[I]    = Cl ? Cl->rawList() : nullptr;
        }
        Queue->ExecuteCommandLists(Submit.CommandListCount, Ptr);
    }

    // Signal phase.
    for (uint32_t I = 0; I < Submit.SignalFenceCount; ++I)
    {
        auto* Fence = d3d12Cast<D3D12Fence>(Submit.SignalFences[I].Fence);
        if (!Fence)
            continue;
        const HRESULT Hr = Queue->Signal(Fence->raw(), Submit.SignalFences[I].Value);
        if (FAILED(Hr))
        {
            GOLETA_LOG_ERROR(D3D12, "Queue::Signal: HRESULT 0x{:08x}", static_cast<unsigned>(Hr));
            return Err{hresultToRhiError(Hr)};
        }
    }

    return {};
}

Result<void, RhiError> D3D12Queue::present(IRhiSwapChain* SwapChain, const RhiFenceWait* WaitFences,
                                           uint32_t WaitFenceCount)
{
    if (!Queue || !SwapChain)
        return Err{RhiError::InvalidArgument};

    for (uint32_t I = 0; I < WaitFenceCount; ++I)
    {
        auto* Fence = d3d12Cast<D3D12Fence>(WaitFences[I].Fence);
        if (!Fence)
            continue;
        const HRESULT Hr = Queue->Wait(Fence->raw(), WaitFences[I].Value);
        if (FAILED(Hr))
            return Err{hresultToRhiError(Hr)};
    }

    auto* Sc = d3d12Cast<D3D12SwapChain>(SwapChain);
    return Sc->present();
}

Result<void, RhiError> D3D12Queue::waitIdle()
{
    if (!Queue || !IdleFence || !IdleEvent)
        return Err{RhiError::Unknown};

    ++IdleValue;
    HRESULT Hr = Queue->Signal(IdleFence.Get(), IdleValue);
    if (FAILED(Hr))
        return Err{hresultToRhiError(Hr)};

    if (IdleFence->GetCompletedValue() >= IdleValue)
        return {};

    Hr = IdleFence->SetEventOnCompletion(IdleValue, IdleEvent);
    if (FAILED(Hr))
        return Err{hresultToRhiError(Hr)};
    ::WaitForSingleObject(IdleEvent, INFINITE);
    return {};
}

} // namespace goleta::rhi::d3d12
