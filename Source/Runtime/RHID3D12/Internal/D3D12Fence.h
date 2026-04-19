#pragma once

/// @file
/// @brief D3D12 implementation of IRhiFence (timeline semantics over ID3D12Fence1).

#include <string>

#include "D3D12Prelude.h"
#include "RHISync.h"

namespace goleta::rhi::d3d12
{

class D3D12Fence final : public IRhiFence
{
public:
    static Rc<D3D12Fence> create(ID3D12Device* Device, uint64_t InitialValue) noexcept;

    ~D3D12Fence() override;

    static constexpr RhiResourceKind kExpectedKind = RhiResourceKind::Fence;

    // IRhiResource
    RhiResourceKind kind() const noexcept override { return kExpectedKind; }
    const char*     debugName() const noexcept override { return Name.c_str(); }
    void            setDebugName(const char* NewName) override;
    RhiNativeHandle nativeHandle() const noexcept override { return {RhiNativeHandleKind::D3D12Fence, Fence.Get()}; }

    // IRhiFence
    uint64_t                        completedValue() const noexcept override;
    Result<RhiWaitStatus, RhiError> wait(uint64_t Value, uint64_t TimeoutNanos) override;
    Result<void, RhiError>          signalFromCpu(uint64_t Value) override;

    ID3D12Fence1* raw() const noexcept { return Fence.Get(); }

private:
    D3D12Fence() noexcept = default;

    ComPtr<ID3D12Fence1> Fence;
    HANDLE               Event = nullptr;
    std::string          Name;
};

} // namespace goleta::rhi::d3d12
