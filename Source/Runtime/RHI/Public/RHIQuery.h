#pragma once

/// @file
/// @brief Query heap for timestamps, occlusion, and pipeline statistics.

#include <cstdint>

#include "RHIExport.h"
#include "RHIResource.h"
#include "RHIStructChain.h"

namespace goleta::rhi
{

enum class RhiQueryKind : uint8_t
{
    Timestamp = 0,
    Occlusion,
    PipelineStatistics,
};

struct RhiQueryHeapDesc
{
    static constexpr RhiStructType kStructType = RhiStructType::QueryHeapDesc;
    RhiStructHeader Header{kStructType, nullptr};

    RhiQueryKind    Kind      = RhiQueryKind::Timestamp;
    uint32_t        Count     = 0;
    const char*     DebugName = nullptr;
};

class RHI_API IRhiQueryHeap : public IRhiResource
{
public:
    virtual const RhiQueryHeapDesc& desc() const noexcept = 0;
    virtual RhiQueryKind            queryKind() const noexcept = 0;
    virtual uint32_t                capacity() const noexcept = 0;
};

} // namespace goleta::rhi
