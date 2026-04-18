#pragma once

/// @file
/// @brief Internal access to the global subsystem factory registry populated by GOLETA_REGISTER_SUBSYSTEM.

#include "Containers/Vec.h"
#include "Subsystem.h"

namespace goleta::detail
{

/// Read-only view of every factory entry registered at static-init time.
const Vec<SubsystemFactoryEntry>& subsystemRegistry();

} // namespace goleta::detail
