/// @file
/// @brief ID3D12Fence1 timeline wrapper.

#include "D3D12Fence.h"

namespace goleta::rhi::d3d12
{

Rc<D3D12Fence> D3D12Fence::create(ID3D12Device* Device, uint64_t InitialValue) noexcept
{
    if (!Device)
        return {};
    auto Self = Rc<D3D12Fence>(new D3D12Fence{});
    const HRESULT Hr = Device->CreateFence(InitialValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&Self->Fence));
    if (FAILED(Hr))
    {
        GOLETA_LOG_ERROR(D3D12, "CreateFence: HRESULT 0x{:08x}", static_cast<unsigned>(Hr));
        return {};
    }
    Self->Event = ::CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS);
    if (!Self->Event)
    {
        GOLETA_LOG_ERROR(D3D12, "CreateEventEx failed: GetLastError={}", static_cast<unsigned>(::GetLastError()));
        return {};
    }
    return Self;
}

D3D12Fence::~D3D12Fence()
{
    if (Event)
    {
        ::CloseHandle(Event);
        Event = nullptr;
    }
}

void D3D12Fence::setDebugName(const char* NewName)
{
    Name = NewName ? NewName : "";
    if (Fence)
        setD3dObjectName(Fence.Get(), Name.c_str());
}

uint64_t D3D12Fence::completedValue() const noexcept
{
    return Fence ? Fence->GetCompletedValue() : 0;
}

Result<RhiWaitStatus, RhiError> D3D12Fence::wait(uint64_t Value, uint64_t TimeoutNanos)
{
    if (!Fence || !Event)
        return Err{RhiError::Unknown};

    if (Fence->GetCompletedValue() >= Value)
        return Ok{RhiWaitStatus::Reached};

    const HRESULT Hr = Fence->SetEventOnCompletion(Value, Event);
    if (FAILED(Hr))
    {
        GOLETA_LOG_ERROR(D3D12, "Fence::SetEventOnCompletion: HRESULT 0x{:08x}", static_cast<unsigned>(Hr));
        return Err{hresultToRhiError(Hr)};
    }

    const DWORD  Millis = nanosecondsToWaitMillis(TimeoutNanos);
    const DWORD  WaitR  = ::WaitForSingleObject(Event, Millis);
    if (WaitR == WAIT_OBJECT_0)
        return Ok{RhiWaitStatus::Reached};
    if (WaitR == WAIT_TIMEOUT)
        return Ok{RhiWaitStatus::TimedOut};
    return Err{RhiError::Unknown};
}

Result<void, RhiError> D3D12Fence::signalFromCpu(uint64_t Value)
{
    if (!Fence)
        return Err{RhiError::Unknown};
    const HRESULT Hr = Fence->Signal(Value);
    if (FAILED(Hr))
    {
        GOLETA_LOG_ERROR(D3D12, "Fence::Signal: HRESULT 0x{:08x}", static_cast<unsigned>(Hr));
        return Err{hresultToRhiError(Hr)};
    }
    return {};
}

} // namespace goleta::rhi::d3d12
