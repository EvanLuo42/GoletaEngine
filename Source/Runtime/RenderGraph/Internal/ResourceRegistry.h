#pragma once

/// @file
/// @brief Per-frame binding table: LogicalResourceId -> IRhiTexture* / IRhiBuffer*.

#include <cstdint>
#include <vector>

#include "Memory/Rc.h"
#include "RHIBuffer.h"
#include "RHITexture.h"
#include "RenderGraphTypes.h"

namespace goleta::rhi
{
class IRhiDevice;
}

namespace goleta::rg
{

struct CompiledGraph;
class  ResourcePool;

/// @brief Holds the physical resources currently bound to each LogicalResourceId. RenderGraph
///        manages its lifecycle: beginFrame sizes the tables and pins Imported/Persistent
///        resources, acquireForGroup runs at the start of each group to materialise transients,
///        releaseAfterGroup runs after the group is recorded so later groups can reuse them
///        via the shared Pool.
class ResourceRegistry
{
public:
    /// @brief Zero-sized the tables and pin Imported / Persistent logicals. Transients are
    ///        filled lazily via acquireForGroup().
    void beginFrame(rhi::IRhiDevice& Device, const CompiledGraph& Compiled, ResourcePool& Pool);

    /// @brief Acquire the transient logicals whose first-use falls in the given group.
    void acquireForGroup(rhi::IRhiDevice& Device, const CompiledGraph& Compiled,
                        const std::vector<uint32_t>& Logicals, ResourcePool& Pool);

    /// @brief Release transient logicals whose last-use falls in the given group. Physical
    ///        resources land back in the pool and become candidates for subsequent groups
    ///        within the same frame.
    void releaseAfterGroup(const CompiledGraph& Compiled,
                           const std::vector<uint32_t>& Logicals, ResourcePool& Pool);

    /// @brief End of frame: push any remaining transients back to the pool. Persistent and
    ///        Imported logicals stay in place.
    void endFrame(const CompiledGraph& Compiled, ResourcePool& Pool) noexcept;

    /// @brief Physical texture bound to @p Id, or nullptr if unset / wrong type.
    [[nodiscard]] rhi::IRhiTexture* texture(LogicalResourceId Id) const noexcept;

    /// @brief Physical buffer bound to @p Id, or nullptr if unset / wrong type.
    [[nodiscard]] rhi::IRhiBuffer* buffer(LogicalResourceId Id) const noexcept;

private:
    std::vector<Rc<rhi::IRhiTexture>> Textures_;
    std::vector<Rc<rhi::IRhiBuffer>>  Buffers_;
};

} // namespace goleta::rg
