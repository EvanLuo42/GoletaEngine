#pragma once

/// @file
/// @brief Timeline-fence interface plus the wait/signal value pairs used by queues.

#include <cstdint>

#include "RHIEnums.h"
#include "RHIExport.h"
#include "RHIResource.h"
#include "Result.h"

namespace goleta::rhi
{

class IRhiFence;

/// @brief A GPU-or-CPU wait on a timeline value.
struct RhiFenceWait
{
    IRhiFence* Fence = nullptr;
    uint64_t   Value = 0;
};

/// @brief A GPU-or-CPU signal raising a timeline value.
struct RhiFenceSignal
{
    IRhiFence* Fence = nullptr;
    uint64_t   Value = 0;
};

/// @brief Timeline semaphore (VK_SEMAPHORE_TYPE_TIMELINE / ID3D12Fence).
///        Binary semaphores are not exposed — every sync is a monotonically increasing uint64.
class RHI_API IRhiFence : public IRhiResource
{
public:
    /// @brief Current value last observed by the device.
    virtual uint64_t completedValue() const noexcept = 0;

    /// @brief Block the calling thread until the fence reaches Value.
    /// @param TimeoutNanos  Max wait in nanoseconds. UINT64_MAX = forever.
    /// @return Ok(Reached) when the target value is observed, Ok(TimedOut) on timeout,
    ///         Err(DeviceLost) if the device died while waiting.
    virtual Result<RhiWaitStatus, RhiError> wait(uint64_t Value, uint64_t TimeoutNanos = ~uint64_t{0}) = 0;

    /// @brief Raise the timeline from the CPU side. Value must be > completedValue().
    virtual Result<void, RhiError> signalFromCpu(uint64_t Value) = 0;
};

} // namespace goleta::rhi
