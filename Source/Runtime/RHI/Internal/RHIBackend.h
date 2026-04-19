#pragma once

/// @file
/// @brief Backend registration. A backend module lives as a sibling (RHID3D12, RHIVulkan,
///        RHIGNMX, …) that links against RHI and registers a factory from one of its TUs
///        via GOLETA_REGISTER_RHI_BACKEND.

#include "Memory/Rc.h"
#include "RHIEnums.h"
#include "RHIExport.h"
#include "RHIInstance.h"

namespace goleta::rhi
{

/// @brief Signature for a backend-provided factory. Returns a null Rc on failure.
using RhiBackendFactoryFn = Rc<IRhiInstance> (*)(const RhiInstanceCreateInfo& Desc);

struct RhiBackendEntry
{
    BackendKind           Kind    = BackendKind::Null;
    RhiBackendFactoryFn   Factory = nullptr;
    const char*           Name    = nullptr;   // Must point to static storage.
};

/// @brief Append a backend to the process-wide registry. Typically called from the static
///        initializer emitted by GOLETA_REGISTER_RHI_BACKEND.
/// @note  The first registration for a given BackendKind wins; subsequent duplicates are
///        ignored (a Full-level debug warning is logged when a callback is installed).
RHI_API void registerRhiBackend(const RhiBackendEntry& Entry);

/// @brief Look up the currently registered factory for Kind. Returns nullptr when none.
RHI_API RhiBackendFactoryFn findRhiBackend(BackendKind Kind) noexcept;

} // namespace goleta::rhi

#ifndef GOLETA_RHI_CONCAT_INNER
#define GOLETA_RHI_CONCAT_INNER(A, B) A##B
#define GOLETA_RHI_CONCAT(A, B)       GOLETA_RHI_CONCAT_INNER(A, B)
#endif

/// @brief Self-registering helper. Place one invocation in a backend's Private/ TU.
///        Example: GOLETA_REGISTER_RHI_BACKEND(Vulkan, &createVulkanInstance);
#define GOLETA_REGISTER_RHI_BACKEND(KindEnum, FactoryFn)                                               \
    namespace                                                                                          \
    {                                                                                                  \
    static const int GOLETA_RHI_CONCAT(_rhi_backend_reg_, __LINE__) = []() {                           \
        ::goleta::rhi::registerRhiBackend(                                                             \
            ::goleta::rhi::RhiBackendEntry{                                                            \
                ::goleta::rhi::BackendKind::KindEnum, (FactoryFn), #KindEnum});                        \
        return 0;                                                                                      \
    }();                                                                                               \
    }
