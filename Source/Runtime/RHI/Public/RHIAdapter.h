#pragma once

/// @file
/// @brief Adapter descriptor returned by IRhiInstance::enumerateAdapters().

#include <cstdint>

#include "RHIEnums.h"

namespace goleta::rhi
{

enum class RhiAdapterKind : uint8_t
{
    Discrete = 0,
    Integrated,
    Software,
    Other,
};

struct RhiAdapterInfo
{
    char           Name[128]                  = {};
    uint32_t       VendorId                   = 0;
    uint32_t       DeviceId                   = 0;
    uint64_t       DedicatedVideoMemoryBytes  = 0;
    uint64_t       DedicatedSystemMemoryBytes = 0;
    uint64_t       SharedSystemMemoryBytes    = 0;
    RhiAdapterKind Kind                       = RhiAdapterKind::Other;
    uint64_t       LuidLow                    = 0; ///< Windows: low 64 bits of LUID. 0 on other platforms.
    uint64_t       LuidHigh                   = 0;
    BackendKind    Backend                    = BackendKind::Null;
};

} // namespace goleta::rhi
