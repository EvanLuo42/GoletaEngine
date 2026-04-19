#pragma once

/// @file
/// @brief Submit / present queue interface.

#include <cstddef>

#include "RHIEnums.h"
#include "RHIExport.h"
#include "RHIResource.h"
#include "RHIStructChain.h"
#include "RHISync.h"
#include "Result.h"

namespace goleta::rhi
{

class IRhiCommandList;
class IRhiSwapChain;

/// @brief Ordered batch of command lists submitted to a queue.
struct RhiSubmitInfo
{
    static constexpr auto kStructType = RhiStructType::SubmitInfo;
    RhiStructHeader       Header{.sType = kStructType, .pNext = nullptr};

    IRhiCommandList* const* CommandLists     = nullptr;
    uint32_t                CommandListCount = 0;

    const RhiFenceWait* WaitFences     = nullptr;
    uint32_t            WaitFenceCount = 0;

    const RhiFenceSignal* SignalFences     = nullptr;
    uint32_t              SignalFenceCount = 0;
};

/// @brief GPU work queue. submit() is serialized internally; multiple producer threads may call.
class RHI_API IRhiQueue : public IRhiResource
{
public:
    /// @brief Which queue family this object services. IRhiResource::kind() still returns
    ///        RhiResourceKind::Queue for runtime type checks.
    [[nodiscard]] virtual RhiQueueKind queueKind() const noexcept = 0;

    /// @brief Submit a batch of command lists. Err(DeviceLost) on TDR / device removal.
    virtual Result<void, RhiError> submit(const RhiSubmitInfo& Submit) = 0;

    /// @brief Present a swap chain image, waiting on the supplied fence values first.
    /// @return Err(OutOfDate) means the caller must recreate the swap chain before the next
    ///         frame. Err(DeviceLost) is terminal.
    virtual Result<void, RhiError> present(IRhiSwapChain* SwapChain, const RhiFenceWait* WaitFences,
                                           uint32_t WaitFenceCount) = 0;

    /// @brief Block until the queue has drained all prior submissions.
    virtual Result<void, RhiError> waitIdle() = 0;
};

} // namespace goleta::rhi
