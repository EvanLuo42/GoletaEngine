#pragma once

/// @file
/// @brief Internal access to the global subsystem factory registry populated by GOLETA_REGISTER_SUBSYSTEM.

#include <vector>

#include "Subsystem.h"

namespace goleta::detail
{

/// Read-only view of every factory entry registered at static-init time.
const std::vector<SubsystemFactoryEntry>& subsystemRegistry();

} // namespace goleta::detail
