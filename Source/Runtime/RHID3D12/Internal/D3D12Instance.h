#pragma once

/// @file
/// @brief IRhiInstance: wraps IDXGIFactory7 + optional ID3D12Debug6; creates a D3D12Device.

#include <vector>

#include "D3D12Prelude.h"
#include "RHIInstance.h"

namespace goleta::rhi::d3d12
{

class D3D12Instance final : public IRhiInstance
{
public:
    static Rc<D3D12Instance> create(const RhiInstanceCreateInfo& Desc) noexcept;

    // IRhiInstance
    BackendKind                   backend() const noexcept override { return BackendKind::D3D12; }
    std::vector<RhiAdapterInfo>   enumerateAdapters() const override;
    Rc<IRhiDevice>                createDevice(const RhiDeviceCreateInfo& Desc) override;

private:
    D3D12Instance() noexcept = default;

    ComPtr<IDXGIFactory7>    Factory;
    ComPtr<ID3D12Debug6>     DebugController;
    RhiDebugLevel            DebugLevel_       = RhiDebugLevel::None;
    RhiDebugCallback         DebugCallback_    = nullptr;
    void*                    DebugUser_        = nullptr;
};

/// @brief Exposed to the registry; not intended for direct use. Implemented in D3D12Backend.cpp.
Rc<IRhiInstance> createD3D12Instance(const RhiInstanceCreateInfo& Desc);

} // namespace goleta::rhi::d3d12
