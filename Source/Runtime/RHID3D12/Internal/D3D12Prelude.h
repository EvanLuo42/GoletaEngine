#pragma once

/// @file
/// @brief Shared D3D12 backend includes plus a handful of HRESULT/logging shims.

#ifndef NOMINMAX
#  define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#  define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>

#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>

#include <cstdint>

#include "Log.h"
#include "RHIEnums.h"
#include "Result.h"

#if defined(GOLETA_RHID3D12_HAS_D3D12MA) && GOLETA_RHID3D12_HAS_D3D12MA
#  include "D3D12MemAlloc.h"
#endif

namespace goleta::rhi::d3d12
{

template <class T>
using ComPtr = Microsoft::WRL::ComPtr<T>;

constexpr const char* kLogCategory = "D3D12";

/// @brief Map an HRESULT onto the RHI error enum. Keeps callers out of the Win32 error space.
inline RhiError hresultToRhiError(HRESULT Hr) noexcept
{
    switch (Hr)
    {
    case S_OK:                      return RhiError::Unknown; // Caller shouldn't invoke on success.
    case E_OUTOFMEMORY:             return RhiError::OutOfMemory;
    case E_INVALIDARG:              return RhiError::InvalidArgument;
    case E_NOTIMPL:                 return RhiError::Unsupported;
    case DXGI_ERROR_DEVICE_REMOVED: return RhiError::DeviceLost;
    case DXGI_ERROR_DEVICE_HUNG:    return RhiError::DeviceLost;
    case DXGI_ERROR_DEVICE_RESET:   return RhiError::DeviceLost;
    case DXGI_ERROR_NOT_FOUND:      return RhiError::NotFound;
    case DXGI_ERROR_UNSUPPORTED:    return RhiError::Unsupported;
    default:                        return RhiError::Unknown;
    }
}

/// @brief Build an Err<RhiError> from an HRESULT and log the failure.
#define GOLETA_D3D12_CHECK_RET(Hr, Where)                                                                     \
    do {                                                                                                      \
        const HRESULT _hr = (Hr);                                                                             \
        if (FAILED(_hr))                                                                                      \
        {                                                                                                     \
            GOLETA_LOG_ERROR(D3D12, "{}: HRESULT 0x{:08x}", (Where), static_cast<unsigned>(_hr));             \
            return Err{::goleta::rhi::d3d12::hresultToRhiError(_hr)};                                         \
        }                                                                                                     \
    } while (0)

#define GOLETA_D3D12_CHECK_NULL(Hr, Where)                                                                    \
    do {                                                                                                      \
        const HRESULT _hr = (Hr);                                                                             \
        if (FAILED(_hr))                                                                                      \
        {                                                                                                     \
            GOLETA_LOG_ERROR(D3D12, "{}: HRESULT 0x{:08x}", (Where), static_cast<unsigned>(_hr));             \
            return {};                                                                                        \
        }                                                                                                     \
    } while (0)

#define GOLETA_D3D12_WARN(Hr, Where)                                                                          \
    do {                                                                                                      \
        const HRESULT _hr = (Hr);                                                                             \
        if (FAILED(_hr))                                                                                      \
        {                                                                                                     \
            GOLETA_LOG_WARN(D3D12, "{}: HRESULT 0x{:08x}", (Where), static_cast<unsigned>(_hr));              \
        }                                                                                                     \
    } while (0)

/// @brief UTF-8 debug-name → wide for SetName.
void setD3dObjectName(ID3D12Object* Object, const char* Utf8Name) noexcept;

/// @brief Nanoseconds → milliseconds for WaitForSingleObject; saturating to INFINITE on overflow.
DWORD nanosecondsToWaitMillis(uint64_t Nanos) noexcept;

/// @brief Downcast an RHI interface pointer to its D3D12 backend impl.
///        Debug: asserts `Ptr->kind() == Derived::kExpectedKind` — catches callers that pass
///        the wrong resource type through the RHI's polymorphic boundary (a class of bug that
///        without RTTI would silently produce UB later).
///        Release: compiles to a bare `static_cast` (zero overhead).
/// @note  The RTTI-off engine forbids `dynamic_cast`; the kind-tag invariant is what keeps
///        backend downcasts safe. Every D3D12 impl class exposes a
///        `static constexpr RhiResourceKind kExpectedKind`.
/// @tparam Derived   Concrete D3D12 class to cast to (e.g. D3D12Buffer).
template <class Derived, class Base>
[[nodiscard]] inline Derived* d3d12Cast(Base* Ptr) noexcept
{
    if (!Ptr)
        return nullptr;
#ifndef NDEBUG
    assert(Ptr->kind() == Derived::kExpectedKind && "d3d12Cast: RhiResourceKind mismatch");
#endif
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
    return static_cast<Derived*>(Ptr);
}

} // namespace goleta::rhi::d3d12
