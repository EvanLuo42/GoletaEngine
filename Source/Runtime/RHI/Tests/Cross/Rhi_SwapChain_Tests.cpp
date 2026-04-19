/// @file
/// @brief Swap-chain creation + back-buffer cycling. Null uses a fake HWND; D3D12 gets a
///        hidden off-screen window. Other backends plug in via RhiNativeWindowKind variants.

#include "Common/RhiTestFixture.h"

#ifdef _WIN32
#  ifndef NOMINMAX
#    define NOMINMAX
#  endif
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  include <windows.h>
#endif

using namespace goleta::rhi::tests;

namespace
{
struct TestWindow
{
    RhiNativeWindow Native{};
#ifdef _WIN32
    HWND Hwnd = nullptr;
#endif

    ~TestWindow()
    {
#ifdef _WIN32
        if (Hwnd) DestroyWindow(Hwnd);
#endif
    }
};

TestWindow acquireTestWindow(BackendKind Kind, uint32_t Width, uint32_t Height)
{
    TestWindow W{};
    W.Native.Kind = RhiNativeWindowKind::Win32Hwnd;

    if (Kind == BackendKind::Null)
    {
        // Null backend does no OS integration; any non-null pointer is accepted.
        W.Native.Handle = reinterpret_cast<void*>(uintptr_t{0xDEADBEEF});
        return W;
    }

#ifdef _WIN32
    static ATOM s_Class = []() {
        WNDCLASSEXW Wc{};
        Wc.cbSize        = sizeof(Wc);
        Wc.style         = CS_OWNDC;
        Wc.lpfnWndProc   = DefWindowProcW;
        Wc.hInstance     = GetModuleHandleW(nullptr);
        Wc.lpszClassName = L"GoletaRhiTestHiddenClass";
        return RegisterClassExW(&Wc);
    }();
    (void)s_Class;
    W.Hwnd = CreateWindowExW(0, L"GoletaRhiTestHiddenClass", L"headless", WS_POPUP, 0, 0,
                              static_cast<int>(Width), static_cast<int>(Height), nullptr, nullptr,
                              GetModuleHandleW(nullptr), nullptr);
    W.Native.Handle = W.Hwnd;
#endif
    return W;
}
} // namespace

GPU_TEST(SwapChain, CreateAndCyclesImages, BackendMask::All)
{
    auto W = acquireTestWindow(F.GetParam(), 256, 256);
    ASSERT_NE(W.Native.Handle, nullptr);

    RhiSwapChainDesc D{};
    D.NativeWindow = W.Native;
    D.Width        = 256;
    D.Height       = 256;
    D.Format       = RhiFormat::Bgra8Unorm;
    D.ImageCount   = 3;
    D.PresentMode  = RhiPresentMode::Immediate;

    auto Sc = F.Device->createSwapChain(D);
    ASSERT_TRUE(Sc);
    EXPECT_GE(Sc->imageCount(), 2u);
    for (uint32_t I = 0; I < Sc->imageCount(); ++I)
        ASSERT_NE(Sc->image(I), nullptr);

    const auto First  = Sc->acquireNextImage();
    const auto Second = Sc->acquireNextImage();
    ASSERT_TRUE(First.isOk());
    ASSERT_TRUE(Second.isOk());
    // Null cycles monotonically; D3D12 returns GetCurrentBackBufferIndex which may repeat if
    // the app hasn't presented between acquires. Either way both return valid indices < count.
    EXPECT_LT(First.value(), Sc->imageCount());
    EXPECT_LT(Second.value(), Sc->imageCount());
}
