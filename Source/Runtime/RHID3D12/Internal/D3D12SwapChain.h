#pragma once

/// @file
/// @brief IRhiSwapChain wrapping IDXGISwapChain4.

#include <string>
#include <vector>

#include "D3D12Prelude.h"
#include "RHISwapChain.h"

namespace goleta::rhi::d3d12
{

class D3D12Device;
class D3D12Queue;
class D3D12Texture;

class D3D12SwapChain final : public IRhiSwapChain
{
public:
    static Rc<D3D12SwapChain> create(D3D12Device* Device, D3D12Queue* PresentQueue, IDXGIFactory7* Factory,
                                     const RhiSwapChainDesc& Desc) noexcept;

    ~D3D12SwapChain() override;

    static constexpr RhiResourceKind kExpectedKind = RhiResourceKind::SwapChain;

    // IRhiResource
    RhiResourceKind kind() const noexcept override { return kExpectedKind; }
    const char*     debugName() const noexcept override { return Name.c_str(); }
    void            setDebugName(const char* NewName) override { Name = NewName ? NewName : ""; }

    // IRhiSwapChain
    const RhiSwapChainDesc&         desc() const noexcept override { return Desc_; }
    uint32_t                        imageCount() const noexcept override { return static_cast<uint32_t>(Images.size()); }
    RhiFormat                       format() const noexcept override { return Desc_.Format; }
    uint32_t                        width() const noexcept override { return Desc_.Width; }
    uint32_t                        height() const noexcept override { return Desc_.Height; }
    IRhiTexture*                    image(uint32_t Index) override;
    uint32_t                        currentImageIndex() const noexcept override { return CurrentIndex; }
    Result<uint32_t, RhiError>      acquireNextImage(const RhiFenceSignal* Signal = nullptr) override;
    void                            resize(uint32_t Width, uint32_t Height) override;

    Result<void, RhiError> present() noexcept;

private:
    D3D12SwapChain() noexcept = default;

    bool rebuildBackBuffers() noexcept;
    void releaseBackBuffers() noexcept;

    D3D12Device*            OwnerDevice  = nullptr;
    D3D12Queue*             PresentQueue = nullptr;
    ComPtr<IDXGISwapChain4> SwapChain;
    RhiSwapChainDesc        Desc_{};
    std::vector<Rc<D3D12Texture>> Images;
    uint32_t                CurrentIndex = 0;
    UINT                    SyncInterval = 1;
    UINT                    PresentFlags = 0;
    bool                    AllowTearing = false;
    std::string             Name;
};

} // namespace goleta::rhi::d3d12
