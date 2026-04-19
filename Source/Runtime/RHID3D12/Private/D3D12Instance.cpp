/// @file
/// @brief D3D12Instance: DXGI factory enumeration + device creation.

#include "D3D12Instance.h"

#include "D3D12Device.h"

#include <cstring>

namespace goleta::rhi::d3d12
{
namespace
{

RhiAdapterInfo toAdapterInfo(IDXGIAdapter4* Adapter) noexcept
{
    RhiAdapterInfo Info{};
    Info.Backend = BackendKind::D3D12;
    if (!Adapter)
        return Info;
    DXGI_ADAPTER_DESC3 Desc{};
    if (FAILED(Adapter->GetDesc3(&Desc)))
        return Info;
    ::WideCharToMultiByte(CP_UTF8, 0, Desc.Description, -1, Info.Name, sizeof(Info.Name), nullptr, nullptr);
    Info.VendorId                   = Desc.VendorId;
    Info.DeviceId                   = Desc.DeviceId;
    Info.DedicatedVideoMemoryBytes  = Desc.DedicatedVideoMemory;
    Info.DedicatedSystemMemoryBytes = Desc.DedicatedSystemMemory;
    Info.SharedSystemMemoryBytes    = Desc.SharedSystemMemory;
    Info.LuidLow                    = static_cast<uint64_t>(Desc.AdapterLuid.LowPart);
    Info.LuidHigh                   = static_cast<uint64_t>(static_cast<uint32_t>(Desc.AdapterLuid.HighPart));
    if (Desc.Flags & DXGI_ADAPTER_FLAG3_SOFTWARE)
        Info.Kind = RhiAdapterKind::Software;
    else
        Info.Kind = (Desc.DedicatedVideoMemory > 0) ? RhiAdapterKind::Discrete : RhiAdapterKind::Integrated;
    return Info;
}

} // namespace

Rc<D3D12Instance> D3D12Instance::create(const RhiInstanceCreateInfo& Desc) noexcept
{
    auto Self = Rc<D3D12Instance>(new D3D12Instance{});
    Self->DebugLevel_    = Desc.DebugLevel;
    Self->DebugCallback_ = Desc.MessageCallback;
    Self->DebugUser_     = Desc.MessageCallbackUser;

    UINT Flags = 0;
    if (Desc.DebugLevel >= RhiDebugLevel::Validation)
    {
        ComPtr<ID3D12Debug6> DebugCtrl;
        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&DebugCtrl))))
        {
            DebugCtrl->EnableDebugLayer();
            if (Desc.DebugLevel >= RhiDebugLevel::GpuAssisted)
                DebugCtrl->SetEnableGPUBasedValidation(TRUE);
            Self->DebugController = std::move(DebugCtrl);
            Flags |= DXGI_CREATE_FACTORY_DEBUG;
        }
    }

    HRESULT Hr = CreateDXGIFactory2(Flags, IID_PPV_ARGS(&Self->Factory));
    if (FAILED(Hr))
    {
        GOLETA_LOG_ERROR(D3D12, "CreateDXGIFactory2: HRESULT 0x{:08x}", static_cast<unsigned>(Hr));
        return {};
    }
    return Self;
}

std::vector<RhiAdapterInfo> D3D12Instance::enumerateAdapters() const
{
    std::vector<RhiAdapterInfo> Out;
    if (!Factory)
        return Out;
    for (UINT I = 0;; ++I)
    {
        ComPtr<IDXGIAdapter1> A1;
        if (Factory->EnumAdapterByGpuPreference(I, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&A1))
            == DXGI_ERROR_NOT_FOUND)
            break;
        ComPtr<IDXGIAdapter4> A4;
        if (FAILED(A1.As(&A4)))
            continue;
        // Verify the adapter can create a D3D12 device at minimum feature level 12_0.
        if (FAILED(D3D12CreateDevice(A4.Get(), D3D_FEATURE_LEVEL_12_0, __uuidof(ID3D12Device), nullptr)))
            continue;
        Out.push_back(toAdapterInfo(A4.Get()));
    }
    return Out;
}

Rc<IRhiDevice> D3D12Instance::createDevice(const RhiDeviceCreateInfo& Desc)
{
    if (!Factory)
        return {};
    ComPtr<IDXGIAdapter4> Chosen;
    RhiAdapterInfo        ChosenInfo{};
    uint32_t              Visited = 0;
    for (UINT I = 0;; ++I)
    {
        ComPtr<IDXGIAdapter1> A1;
        if (Factory->EnumAdapterByGpuPreference(I, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&A1))
            == DXGI_ERROR_NOT_FOUND)
            break;
        ComPtr<IDXGIAdapter4> A4;
        if (FAILED(A1.As(&A4)))
            continue;
        if (FAILED(D3D12CreateDevice(A4.Get(), D3D_FEATURE_LEVEL_12_0, __uuidof(ID3D12Device), nullptr)))
            continue;
        if (Desc.AdapterIndex == ~uint32_t{0} || Desc.AdapterIndex == Visited)
        {
            Chosen     = std::move(A4);
            ChosenInfo = toAdapterInfo(Chosen.Get());
            break;
        }
        ++Visited;
    }
    if (!Chosen)
    {
        GOLETA_LOG_ERROR(D3D12, "No suitable D3D12 adapter for index {}", Desc.AdapterIndex);
        return {};
    }
    return D3D12Device::create(Factory, Chosen, ChosenInfo, Desc, DebugLevel_, DebugCallback_, DebugUser_);
}

Rc<IRhiInstance> createD3D12Instance(const RhiInstanceCreateInfo& Desc)
{
    return D3D12Instance::create(Desc);
}

} // namespace goleta::rhi::d3d12
