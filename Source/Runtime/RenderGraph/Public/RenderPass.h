#pragma once

/// @file
/// @brief IRenderPass base class: reflect(), optional compile(), execute().

#include <string>

#include "Memory/Rc.h"
#include "RHIEnums.h"
#include "RenderGraphExport.h"
#include "RenderGraphTypes.h"

namespace goleta::rg
{

class RenderPassReflection;
class RenderContext;
class RenderData;

/// @brief Base class for a render pass. Pass authors subclass this and implement reflect()
///        and execute(); compile() is optional and useful for building pipeline objects once
///        the final extents are known.
class RENDERGRAPH_API IRenderPass : public RefCounted
{
public:
    ~IRenderPass() override = default;

    /// @brief Declare inputs / outputs / inouts. Called exactly once by RenderGraph::addPass.
    virtual void reflect(RenderPassReflection& R) = 0;

    /// @brief Optional one-time hook after the graph has resolved final resource sizes but
    ///        before the first execute(). Default: no-op. Use to construct pipelines, bind
    ///        shader permutations to the resolved dimensions, etc.
    virtual void compile(RenderContext& /*Ctx*/, const RenderPassReflection& /*Final*/) {}

    /// @brief Record the pass's GPU work onto Ctx.cmdList() using resources resolved through Rd.
    virtual void execute(RenderContext& Ctx, const RenderData& Rd) = 0;

    /// @brief Queue family this pass would like to run on. The graph uses this as a hint when
    ///        splitting passes into recording groups; if the device lacks a dedicated queue of
    ///        the requested kind, the pass falls back to Graphics.
    /// @note  Default: Graphics. Override to return Compute for GPU-driven passes that benefit
    ///        from overlapping graphics work, or Copy for large uploads.
    [[nodiscard]] virtual rhi::RhiQueueKind preferredQueue() const noexcept
    {
        return rhi::RhiQueueKind::Graphics;
    }

    /// @brief Human-readable name assigned by RenderGraph::addPass.
    [[nodiscard]] const char* debugName() const noexcept { return Name_.c_str(); }

    /// @brief Per-graph stable ID assigned by RenderGraph::addPass.
    [[nodiscard]] PassId id() const noexcept { return Id_; }

private:
    friend class RenderGraph;
    std::string Name_;
    PassId      Id_ = PassId::Invalid;
};

/// @brief Falcor-style readability alias. Pass authors typically write `class X : public RenderPass`.
using RenderPass = IRenderPass;

} // namespace goleta::rg
