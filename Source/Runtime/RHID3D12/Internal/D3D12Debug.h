#pragma once

/// @file
/// @brief IRhiDebug implementation. Stores the RHI-side callback and coordinates with the D3D12
///        InfoQueue when a Validation / GpuAssisted / Full debug level was requested.

#include <mutex>
#include <string>

#include "D3D12Prelude.h"
#include "RHIDebug.h"

namespace goleta::rhi::d3d12
{

class D3D12Debug final : public IRhiDebug
{
public:
    static Rc<D3D12Debug> create(ID3D12Device* Device, RhiDebugLevel Level, RhiDebugCallback Callback,
                                 void* User) noexcept;

    ~D3D12Debug() override;

    // IRhiDebug
    RhiDebugLevel level() const noexcept override { return Level_; }
    void          setMessageCallback(RhiDebugCallback Callback, void* User) override;
    void          pushMessageFilter(uint32_t MessageId, bool Allow) override;
    void          popMessageFilter() override;
    void          insertBreadcrumb(const char* Label) override;
    bool          tryGetLastCrashReport(RhiCrashReport& OutReport) override;
    void          beginCapture(const char* Name) override;
    void          endCapture() override;

    /// @brief Forward a single D3D12 InfoQueue message to the installed callback.
    void dispatchMessage(const RhiDebugMessage& Message) noexcept;

private:
    D3D12Debug() noexcept = default;

    ComPtr<ID3D12InfoQueue1> InfoQueue;
    DWORD                    CallbackCookie = 0;

    RhiDebugLevel    Level_       = RhiDebugLevel::None;
    RhiDebugCallback Callback_    = nullptr;
    void*            CallbackUser = nullptr;
    std::mutex       CallbackMutex;

    std::string LastBreadcrumb;
    uint32_t    FilterDepth = 0;
};

} // namespace goleta::rhi::d3d12
