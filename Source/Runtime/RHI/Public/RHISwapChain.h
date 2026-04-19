#pragma once

/// @file
/// @brief Swap chain interface and creation descriptor.

#include <cstdint>

#include "RHIEnums.h"
#include "RHIExport.h"
#include "RHIFormat.h"
#include "RHIResource.h"
#include "RHIStructChain.h"
#include "RHISync.h"
#include "Result.h"

namespace goleta::rhi
{

class IRhiTexture;

enum class RhiPresentMode : uint8_t
{
    Immediate = 0, ///< Tearing. Lowest latency.
    Vsync,         ///< Standard vsync.
    Mailbox,       ///< Vsync without blocking the producer (triple-buffered).
};

/// @brief Kind of native window handle. Determines what Handle / Display point at.
enum class RhiNativeWindowKind : uint8_t
{
    Unknown = 0,
    Win32Hwnd,           ///< Handle = HWND.
    X11Window,           ///< Handle = Window (XID), Display = Display*.
    XcbWindow,           ///< Handle = xcb_window_t packed, Display = xcb_connection_t*.
    WaylandSurface,      ///< Handle = wl_surface*, Display = wl_display*.
    AndroidNativeWindow, ///< Handle = ANativeWindow*.
    MetalLayer,          ///< Handle = CAMetalLayer*.
};

/// @brief Discriminated native-window descriptor. Caller fills according to Kind; backends that
///        don't understand Kind fail swap-chain creation with RhiError::Unsupported.
struct RhiNativeWindow
{
    RhiNativeWindowKind Kind    = RhiNativeWindowKind::Unknown;
    void*               Handle  = nullptr; ///< Primary handle; see Kind for interpretation.
    void*               Display = nullptr; ///< Secondary (connection / display). Nullable.
};

struct RhiSwapChainDesc
{
    static constexpr auto kStructType = RhiStructType::SwapChainDesc;
    RhiStructHeader       Header{kStructType, nullptr};

    RhiNativeWindow NativeWindow{};
    uint32_t        Width       = 0;
    uint32_t        Height      = 0;
    RhiFormat       Format      = RhiFormat::Bgra8Unorm;
    uint32_t        ImageCount  = 2;
    RhiPresentMode  PresentMode = RhiPresentMode::Vsync;
    bool            HdrOutput   = false; ///< Requires features().HdrOutput.Supported.

    const char* DebugName = nullptr;
};

class RHI_API IRhiSwapChain : public IRhiResource
{
public:
    virtual const RhiSwapChainDesc& desc() const noexcept = 0;

    virtual uint32_t  imageCount() const noexcept = 0;
    virtual RhiFormat format() const noexcept     = 0;
    virtual uint32_t  width() const noexcept      = 0;
    virtual uint32_t  height() const noexcept     = 0;

    /// @brief Retrieve the texture the GPU should render into for a given back-buffer slot.
    virtual IRhiTexture* image(uint32_t Index) = 0;

    /// @brief Current back buffer index. Valid between acquireNextImage() and present().
    virtual uint32_t currentImageIndex() const noexcept = 0;

    /// @brief Acquire the next back buffer.
    /// @param Signal Optional fence+value the backend raises once the image is safe for GPU
    ///               use. The subsequent submit that renders to this image should wait on it.
    ///               Null on backends that don't need cross-queue synchronization (D3D12).
    /// @return Ok(index) on success, Err(OutOfDate / DeviceLost / Timeout / ...) on failure.
    virtual Result<uint32_t, RhiError> acquireNextImage(const RhiFenceSignal* Signal = nullptr) = 0;

    /// @brief Resize to the new dimensions. All previously returned image() pointers become invalid.
    virtual void resize(uint32_t Width, uint32_t Height) = 0;
};

} // namespace goleta::rhi
