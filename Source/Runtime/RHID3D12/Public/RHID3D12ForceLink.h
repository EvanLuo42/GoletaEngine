#pragma once

/// @file
/// @brief Keep the RHID3D12 TU containing GOLETA_REGISTER_RHI_BACKEND from being dropped by
///        the linker under /OPT:REF when building static engines.

#include "RHID3D12Export.h"

namespace goleta::rhi
{

/// @brief Touch-point to force static-archive inclusion of the D3D12 backend registration TU.
/// @note  Calling this once from any TU that also links RHID3D12 guarantees the backend
///        registers. Shared builds do not need it (the DLL's implicit load runs static init),
///        but calling it is harmless.
RHID3D12_API void forceLinkRhiD3D12Backend();

} // namespace goleta::rhi
