/// @file
/// @brief IDXGISwapChain4 wrapper + back-buffer wrapping as D3D12Texture.

#include "D3D12SwapChain.h"

#include "D3D12Device.h"
#include "D3D12FormatTable.h"
#include "D3D12Queue.h"
#include "D3D12Texture.h"

namespace goleta::rhi::d3d12
{

Rc<D3D12SwapChain> D3D12SwapChain::create(D3D12Device* Device, D3D12Queue* PresentQueue, IDXGIFactory7* Factory,
                                          const RhiSwapChainDesc& Desc) noexcept
{
    if (!Device || !PresentQueue || !Factory)
        return {};
    if (Desc.NativeWindow.Kind != RhiNativeWindowKind::Win32Hwnd || !Desc.NativeWindow.Handle)
    {
        GOLETA_LOG_ERROR(D3D12, "SwapChain requires Win32 HWND handle");
        return {};
    }

    auto Self         = Rc<D3D12SwapChain>(new D3D12SwapChain{});
    Self->OwnerDevice  = Device;
    Self->PresentQueue = PresentQueue;
    Self->Desc_        = Desc;

    // Tearing support check.
    BOOL TearingAllowed = FALSE;
    Factory->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &TearingAllowed, sizeof(TearingAllowed));
    Self->AllowTearing = TearingAllowed == TRUE;

    DXGI_SWAP_CHAIN_DESC1 D{};
    D.Width       = Desc.Width;
    D.Height      = Desc.Height;
    D.Format      = toDxgi(Desc.Format);
    D.Stereo      = FALSE;
    D.SampleDesc.Count = 1;
    D.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    D.BufferCount = Desc.ImageCount == 0 ? 2 : Desc.ImageCount;
    D.Scaling     = DXGI_SCALING_NONE;
    D.SwapEffect  = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    D.AlphaMode   = DXGI_ALPHA_MODE_IGNORE;
    D.Flags       = Self->AllowTearing && Desc.PresentMode == RhiPresentMode::Immediate
                        ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING
                        : 0;

    ComPtr<IDXGISwapChain1> Sc1;
    HRESULT Hr = Factory->CreateSwapChainForHwnd(PresentQueue->raw(),
                                                 static_cast<HWND>(Desc.NativeWindow.Handle), &D, nullptr, nullptr,
                                                 &Sc1);
    if (FAILED(Hr))
    {
        GOLETA_LOG_ERROR(D3D12, "CreateSwapChainForHwnd: HRESULT 0x{:08x}", static_cast<unsigned>(Hr));
        return {};
    }
    Hr = Sc1.As(&Self->SwapChain);
    if (FAILED(Hr))
        return {};

    switch (Desc.PresentMode)
    {
    case RhiPresentMode::Vsync:     Self->SyncInterval = 1; Self->PresentFlags = 0; break;
    case RhiPresentMode::Mailbox:   Self->SyncInterval = 0; Self->PresentFlags = 0; break;
    case RhiPresentMode::Immediate:
        Self->SyncInterval = 0;
        Self->PresentFlags = Self->AllowTearing ? DXGI_PRESENT_ALLOW_TEARING : 0;
        break;
    }
    Factory->MakeWindowAssociation(static_cast<HWND>(Desc.NativeWindow.Handle), DXGI_MWA_NO_ALT_ENTER);

    if (!Self->rebuildBackBuffers())
        return {};

    return Self;
}

D3D12SwapChain::~D3D12SwapChain()
{
    releaseBackBuffers();
}

bool D3D12SwapChain::rebuildBackBuffers() noexcept
{
    releaseBackBuffers();
    DXGI_SWAP_CHAIN_DESC1 D{};
    SwapChain->GetDesc1(&D);
    Images.reserve(D.BufferCount);
    RhiTextureDesc ImgDesc{};
    ImgDesc.Dimension          = RhiTextureDimension::Tex2D;
    ImgDesc.Format             = Desc_.Format;
    ImgDesc.Width              = D.Width;
    ImgDesc.Height             = D.Height;
    ImgDesc.DepthOrArrayLayers = 1;
    ImgDesc.MipLevels          = 1;
    ImgDesc.Samples            = RhiSampleCount::X1;
    ImgDesc.Usage              = RhiTextureUsage::ColorAttachment | RhiTextureUsage::CopySource;
    ImgDesc.Location           = RhiMemoryLocation::DeviceLocal;
    for (UINT I = 0; I < D.BufferCount; ++I)
    {
        ComPtr<ID3D12Resource> Buf;
        if (FAILED(SwapChain->GetBuffer(I, IID_PPV_ARGS(&Buf))))
            return false;
        Images.push_back(D3D12Texture::wrapRaw(OwnerDevice, std::move(Buf), ImgDesc));
    }
    Desc_.Width      = D.Width;
    Desc_.Height     = D.Height;
    Desc_.ImageCount = D.BufferCount;
    CurrentIndex     = SwapChain->GetCurrentBackBufferIndex();
    return true;
}

void D3D12SwapChain::releaseBackBuffers() noexcept
{
    Images.clear();
}

IRhiTexture* D3D12SwapChain::image(uint32_t Index)
{
    return Index < Images.size() ? Images[Index].get() : nullptr;
}

Result<uint32_t, RhiError> D3D12SwapChain::acquireNextImage(const RhiFenceSignal* /*Signal*/)
{
    if (!SwapChain)
        return Err{RhiError::Unknown};
    CurrentIndex = SwapChain->GetCurrentBackBufferIndex();
    return Ok{CurrentIndex};
}

void D3D12SwapChain::resize(uint32_t Width, uint32_t Height)
{
    if (!SwapChain || (Width == Desc_.Width && Height == Desc_.Height))
        return;
    PresentQueue->waitIdle();
    releaseBackBuffers();
    DXGI_SWAP_CHAIN_DESC1 D{};
    SwapChain->GetDesc1(&D);
    SwapChain->ResizeBuffers(D.BufferCount, Width, Height, D.Format, D.Flags);
    Desc_.Width  = Width;
    Desc_.Height = Height;
    rebuildBackBuffers();
}

Result<void, RhiError> D3D12SwapChain::present() noexcept
{
    if (!SwapChain)
        return Err{RhiError::Unknown};
    const HRESULT Hr = SwapChain->Present(SyncInterval, PresentFlags);
    if (FAILED(Hr))
        return Err{hresultToRhiError(Hr)};
    CurrentIndex = SwapChain->GetCurrentBackBufferIndex();
    return {};
}

} // namespace goleta::rhi::d3d12
