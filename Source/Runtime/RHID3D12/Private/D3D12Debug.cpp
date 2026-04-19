/// @file
/// @brief ID3D12InfoQueue1 callback plumbing into the RHI debug callback.

#include "D3D12Debug.h"

namespace goleta::rhi::d3d12
{
namespace
{

RhiDebugSeverity fromD3dSeverity(D3D12_MESSAGE_SEVERITY S) noexcept
{
    switch (S)
    {
    case D3D12_MESSAGE_SEVERITY_CORRUPTION: return RhiDebugSeverity::Corruption;
    case D3D12_MESSAGE_SEVERITY_ERROR:      return RhiDebugSeverity::Error;
    case D3D12_MESSAGE_SEVERITY_WARNING:    return RhiDebugSeverity::Warning;
    case D3D12_MESSAGE_SEVERITY_INFO:       return RhiDebugSeverity::Info;
    case D3D12_MESSAGE_SEVERITY_MESSAGE:    return RhiDebugSeverity::Verbose;
    }
    return RhiDebugSeverity::Info;
}

RhiDebugCategory fromD3dCategory(D3D12_MESSAGE_CATEGORY C) noexcept
{
    switch (C)
    {
    case D3D12_MESSAGE_CATEGORY_APPLICATION_DEFINED:
    case D3D12_MESSAGE_CATEGORY_MISCELLANEOUS:          return RhiDebugCategory::General;
    case D3D12_MESSAGE_CATEGORY_INITIALIZATION:
    case D3D12_MESSAGE_CATEGORY_CLEANUP:                return RhiDebugCategory::StateCreation;
    case D3D12_MESSAGE_CATEGORY_STATE_CREATION:
    case D3D12_MESSAGE_CATEGORY_STATE_SETTING:
    case D3D12_MESSAGE_CATEGORY_STATE_GETTING:          return RhiDebugCategory::StateCreation;
    case D3D12_MESSAGE_CATEGORY_RESOURCE_MANIPULATION:
    case D3D12_MESSAGE_CATEGORY_COMPILATION:            return RhiDebugCategory::ShaderCompiler;
    case D3D12_MESSAGE_CATEGORY_EXECUTION:              return RhiDebugCategory::Performance;
    case D3D12_MESSAGE_CATEGORY_SHADER:                 return RhiDebugCategory::ShaderCompiler;
    }
    return RhiDebugCategory::General;
}

void CALLBACK infoQueueCallback(D3D12_MESSAGE_CATEGORY Category, D3D12_MESSAGE_SEVERITY Severity, D3D12_MESSAGE_ID Id,
                                LPCSTR Description, void* Context)
{
    auto* Self = static_cast<D3D12Debug*>(Context);
    if (!Self) return;
    RhiDebugMessage M{};
    M.Severity  = fromD3dSeverity(Severity);
    M.Category  = fromD3dCategory(Category);
    M.MessageId = static_cast<uint32_t>(Id);
    M.Message   = Description ? Description : "";
    Self->dispatchMessage(M);
}

} // namespace

Rc<D3D12Debug> D3D12Debug::create(ID3D12Device* Device, RhiDebugLevel Level, RhiDebugCallback Callback,
                                  void* User) noexcept
{
    auto Self = Rc<D3D12Debug>(new D3D12Debug{});
    Self->Level_       = Level;
    Self->Callback_    = Callback;
    Self->CallbackUser = User;
    if (Device && Level >= RhiDebugLevel::Validation)
    {
        if (SUCCEEDED(Device->QueryInterface(IID_PPV_ARGS(&Self->InfoQueue))))
        {
            Self->InfoQueue->RegisterMessageCallback(infoQueueCallback, D3D12_MESSAGE_CALLBACK_FLAG_NONE, Self.get(),
                                                     &Self->CallbackCookie);
        }
    }
    return Self;
}

D3D12Debug::~D3D12Debug()
{
    if (InfoQueue && CallbackCookie)
        InfoQueue->UnregisterMessageCallback(CallbackCookie);
}

void D3D12Debug::setMessageCallback(RhiDebugCallback Callback, void* User)
{
    std::scoped_lock Lock(CallbackMutex);
    Callback_    = Callback;
    CallbackUser = User;
}

void D3D12Debug::pushMessageFilter(uint32_t /*MessageId*/, bool /*Allow*/)
{
    ++FilterDepth;
    // TODO(rhi): wire into InfoQueue::PushStorageFilter.
}

void D3D12Debug::popMessageFilter()
{
    if (FilterDepth > 0)
        --FilterDepth;
}

void D3D12Debug::insertBreadcrumb(const char* Label)
{
    LastBreadcrumb = Label ? Label : "";
}

bool D3D12Debug::tryGetLastCrashReport(RhiCrashReport& /*OutReport*/)
{
    // TODO(rhi): DRED-analog retrieval.
    return false;
}

void D3D12Debug::beginCapture(const char* /*Name*/) {}
void D3D12Debug::endCapture() {}

void D3D12Debug::dispatchMessage(const RhiDebugMessage& Message) noexcept
{
    RhiDebugCallback Cb   = nullptr;
    void*            User = nullptr;
    {
        std::scoped_lock Lock(CallbackMutex);
        Cb   = Callback_;
        User = CallbackUser;
    }
    if (Cb)
        Cb(Message, User);
}

} // namespace goleta::rhi::d3d12
