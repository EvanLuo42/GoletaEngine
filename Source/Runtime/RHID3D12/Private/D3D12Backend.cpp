/// @file
/// @brief Self-registering hook into RHI's backend registry.

#include "D3D12Instance.h"
#include "RHID3D12ForceLink.h"
#include "RHIBackend.h"

namespace goleta::rhi
{

void forceLinkRhiD3D12Backend()
{
    // Intentionally empty. Referencing this symbol from another TU keeps the linker from
    // dropping the static registration below under /OPT:REF in static-archive builds.
}

} // namespace goleta::rhi

GOLETA_REGISTER_RHI_BACKEND(D3D12, &::goleta::rhi::d3d12::createD3D12Instance)
