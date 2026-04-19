#pragma once

/// @file
/// @brief Debug-layer interface: validation level, message callback, marker scopes, breadcrumbs,
///        and capture tool bridge. Level and callback are fixed at instance creation.

#include <cstdint>

#include "Memory/Rc.h"
#include "RHIExport.h"
#include "RHIResource.h"
#include "RHIStructChain.h"

namespace goleta::rhi
{

/// @brief Stepped validation / instrumentation level. Lower is cheaper; pick None for shipping.
enum class RhiDebugLevel : uint8_t
{
    None        = 0, ///< No overhead. Release / shipping default.
    Basic       = 1, ///< Object naming and command-list debug scopes only.
    Validation  = 2, ///< + API validation layer (D3D12 Debug Layer / VK Validation Layers).
    GpuAssisted = 3, ///< + GPU-side validation (GBV / VK_EXT_validation_features).
    Full        = 4, ///< + breadcrumbs on submit + DRED-analog crash dump + shader debug info expected.
};

enum class RhiDebugSeverity : uint8_t
{
    Verbose = 0,
    Info,
    Warning,
    Error,
    Corruption,
};

enum class RhiDebugCategory : uint8_t
{
    General = 0,
    Validation,
    Performance,
    ShaderCompiler,
    StateCreation,
    Binding,
    Synchronization,
};

/// @brief One message emitted by the backend validation layer.
/// @note  All pointers are valid only for the duration of the callback invocation.
struct RhiDebugMessage
{
    RhiDebugSeverity Severity  = RhiDebugSeverity::Info;
    RhiDebugCategory Category  = RhiDebugCategory::General;
    uint32_t         MessageId = 0;       ///< Backend-stable id, usable with push/popMessageFilter.
    const char*      Message   = nullptr; ///< UTF-8, null-terminated.
    IRhiResource*    Object    = nullptr; ///< Nullable; the resource the message is about.
};

/// @brief Debug message callback signature. May be invoked from any thread the backend picks
///        (often a driver worker). Implementations must be thread-safe.
using RhiDebugCallback = void (*)(const RhiDebugMessage& Message, void* User);

/// @brief Post-mortem crash report produced after a device-lost / TDR event.
struct RhiCrashReport
{
    const char* Summary             = nullptr;
    const char* LastBreadcrumb      = nullptr;
    const char* AutoBreadcrumbLog   = nullptr; ///< Backend-specific multi-line dump.
    uint32_t    DeviceRemovedReason = 0;
};

/// @brief Optional instance-creation tuning for the debug layer. Chained via pNext.
struct RhiDebugLayerSettings
{
    static constexpr RhiStructType kStructType = RhiStructType::DebugLayerSettings;
    RhiStructHeader                Header{kStructType, nullptr};

    bool EnableShaderDebugInfo = false; ///< Request -Zi style shader PDBs where supported.
    bool EnableSynchronization = false; ///< Sync validation (VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION).
    bool EnableBestPractices   = false; ///< Best-practices validation layer.
    bool EnableDred            = false; ///< Auto-enable DRED / NV device diagnostics checkpoints.
    bool BreakOnError          = false; ///< Debug-break in the validation callback on Error/Corruption.
};

/// @brief Debug facade reached via IRhiDevice::debug(). When the instance was created with
///        RhiDebugLevel::None, returns a stub with level() == None and all methods as no-ops,
///        so callers never have to branch.
class RHI_API IRhiDebug : public RefCounted
{
public:
    virtual RhiDebugLevel level() const noexcept = 0;

    /// @brief Replace the instance-level message callback installed at creation time.
    virtual void setMessageCallback(RhiDebugCallback Callback, void* User) = 0;

    /// @brief Suppress or force-allow a specific message id for the remainder of the scope
    ///        ended by the next popMessageFilter(). Stacks.
    virtual void pushMessageFilter(uint32_t MessageId, bool Allow) = 0;
    virtual void popMessageFilter()                                = 0;

    /// @brief Insert a CPU-timeline breadcrumb. Pairs with tryGetLastCrashReport() on device lost.
    virtual void insertBreadcrumb(const char* Label) = 0;

    /// @brief Retrieve the last crash report. Returns false when no device-lost event has occurred.
    virtual bool tryGetLastCrashReport(RhiCrashReport& OutReport) = 0;

    /// @brief Trigger a programmatic PIX / RenderDoc / Nsight capture. No-op when no tool is attached.
    virtual void beginCapture(const char* Name) = 0;
    virtual void endCapture()                   = 0;
};

} // namespace goleta::rhi
