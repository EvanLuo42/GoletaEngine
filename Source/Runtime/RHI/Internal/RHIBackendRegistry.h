#pragma once

/// @file
/// @brief Internal access to the backend registry — used by RHIInstance.cpp to dispatch
///        createInstance() and by tests that need to reset state between cases.

#include "RHIBackend.h"

namespace goleta::rhi::detail
{

/// @brief Reset the registry to empty. Test-only; never call from shipping code.
RHI_API void resetRhiBackendRegistryForTests();

/// @brief Total number of backends currently registered.
RHI_API uint32_t rhiBackendRegistrationCount() noexcept;

} // namespace goleta::rhi::detail
