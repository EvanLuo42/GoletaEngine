#pragma once

/// @file
/// @brief RenderGraph: add passes, connect them with typed handles, compile, execute.

#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>

#include "Memory/Rc.h"
#include "PassField.h"
#include "RHIBarrier.h"
#include "RenderGraphExport.h"
#include "RenderGraphTypes.h"
#include "RenderPass.h"
#include "Result.h"

namespace goleta::rhi
{
class IRhiDevice;
class IRhiTexture;
class IRhiBuffer;
} // namespace goleta::rhi

namespace goleta::rg
{

class RenderContext;

/// @brief Directed-acyclic graph of render passes with typed input / output wiring.
///        Owns physical resources for its transient fields; pass instances are refcounted
///        and released on graph destruction.
class RENDERGRAPH_API RenderGraph
{
public:
    explicit RenderGraph(std::string DebugName = {});
    ~RenderGraph();

    RenderGraph(const RenderGraph&)            = delete;
    RenderGraph& operator=(const RenderGraph&) = delete;
    RenderGraph(RenderGraph&&)                 = delete;
    RenderGraph& operator=(RenderGraph&&)      = delete;

    /// @brief Construct a pass in place, call its reflect(), and register it with the graph.
    /// @return Non-owning raw pointer to the pass; graph holds the owning Rc.
    template <class TPass, class... Args>
    TPass* addPass(std::string_view Name, Args&&... A)
    {
        static_assert(std::is_base_of_v<IRenderPass, TPass>,
                      "TPass must derive from goleta::rg::IRenderPass");
        Rc<TPass> Owned = makeRc<TPass>(std::forward<Args>(A)...);
        TPass* Raw = Owned.get();
        Rc<IRenderPass> AsBase{static_cast<IRenderPass*>(Raw)};
        registerPass(std::move(AsBase), Name);
        return Raw;
    }

    /// @brief Connect a producer output to a consumer input. Same resource tag T enforced at
    ///        compile time; direction mismatch is also a compile error (output/output or
    ///        input/input won't satisfy this template).
    template <class T>
    void connect(PassOutput<T> Src, PassInput<T> Dst)
    {
        constexpr RgResourceType Rt =
            std::is_same_v<T, Texture> ? RgResourceType::Texture : RgResourceType::Buffer;
        static_assert(std::is_same_v<T, Texture> || std::is_same_v<T, Buffer>,
                      "connect<T>: T must be rg::Texture or rg::Buffer");
        connectImpl(Src.Pass, Src.Field, Dst.Pass, Dst.Field, Rt);
    }

    /// @brief Mark a texture output as a graph-level terminal (will be transitioned to
    ///        @p Terminal at end of execute).
    template <class T>
    void markOutput(PassOutput<T> H,
                    rhi::RhiTextureLayout Terminal = rhi::RhiTextureLayout::ShaderResource)
    {
        markOutputImpl(H.Pass, H.Field, Terminal);
    }

    /// @brief Overload for an InputOutput handle.
    template <class T>
    void markOutput(PassInputOutput<T> H,
                    rhi::RhiTextureLayout Terminal = rhi::RhiTextureLayout::ShaderResource)
    {
        markOutputImpl(H.Pass, H.Field, Terminal);
    }

    /// @brief Bind an imported texture to a named input. Must be called before compile().
    void setInput(std::string_view PassName, std::string_view FieldName,
                  Rc<rhi::IRhiTexture>  Imported,
                  rhi::RhiTextureLayout InitialLayout = rhi::RhiTextureLayout::Undefined,
                  rhi::RhiAccess        InitialAccess = rhi::RhiAccess::None);

    /// @brief Bind an imported buffer to a named input.
    void setInput(std::string_view PassName, std::string_view FieldName,
                  Rc<rhi::IRhiBuffer> Imported);

    /// @brief Validate the graph, resolve edges, topologically sort, allocate transient
    ///        resources, and plan barriers. Safe to call more than once (re-compiles).
    /// @note  Phase 1 does not require a device for compile-time work (transient resources are
    ///        allocated lazily during execute). @p Device may be null; later phases that plan
    ///        memory aliasing will require a non-null device.
    Result<void, RgError> compile(rhi::IRhiDevice* Device);

    /// @brief Execute the compiled schedule. Acquires command lists from per-thread pools the
    ///        graph owns, submits each group on its preferred queue, and synchronises
    ///        cross-queue dependencies via a graph-owned timeline fence.
    ///        compile() must have succeeded since the last mutation.
    void execute(RenderContext& Ctx);

    /// @brief Install an enkiTS task scheduler. When non-null, group recording is dispatched
    ///        across its worker threads; submit order and cross-queue sync stay single-threaded.
    ///        Passing nullptr restores sequential single-thread recording.
    /// @note  The pointer type is opaque (void*) to keep enkiTS out of the public headers;
    ///        pass the address of your enki::TaskScheduler instance.
    void setTaskScheduler(void* EnkiTaskScheduler) noexcept;

    /// @brief Ask the debug layer to capture the next execute() as a PIX / Nsight / RenderDoc
    ///        frame. No-op when no capture tool is attached. One-shot; resets after firing.
    /// @param Name Shown in the tool's capture list. Empty falls back to the graph's debug name.
    void captureNextFrame(std::string Name = {}) noexcept;

    /// @brief Per-pass timing for the most-recently-executed frame for which data is available.
    struct PassTiming
    {
        PassId      Pass         = PassId::Invalid;
        const char* Name         = nullptr;  ///< Pointer into the graph's owned pass-name storage.
        uint64_t    CpuStartNs   = 0;        ///< Steady-clock nanoseconds at start of execute().
        uint64_t    CpuEndNs     = 0;
        uint64_t    GpuStartTick = 0;        ///< Raw queue-timestamp ticks. Zero if unresolved.
        uint64_t    GpuEndTick   = 0;
    };

    /// @brief Enable or disable per-pass timing collection. Defaults to disabled.
    void setTimingEnabled(bool On) noexcept;

    /// @brief Timings from the most recent execute() where GPU results have been resolved.
    ///        GPU ticks are up to one frame stale (readback happens on the next execute).
    /// @return Empty span when timing is disabled or no frame has been resolved yet.
    [[nodiscard]] std::span<const PassTiming> lastFrameTimings() const noexcept;

    /// @brief Pass debug names, ordered by PassId.
    [[nodiscard]] std::span<const std::string> passNames() const noexcept;

    /// @brief Whether compile() has succeeded since the last mutation.
    [[nodiscard]] bool isCompiled() const noexcept;

    /// @brief Graph-wide debug label.
    [[nodiscard]] const std::string& debugName() const noexcept;

    /// @brief Opaque implementation handle; its definition lives in the module's Private/
    ///        sources. Intentionally public so Internal/ helpers can reference it, but the
    ///        struct itself is not part of the public API.
    struct Impl;

    /// @brief Test-only accessor for the frozen CompiledGraph. Returns nullptr before a
    ///        successful compile(). Consumers must include Internal/CompiledGraph.h.
    /// @note  Not part of the stable API surface; intended for unit-test assertions.
    [[nodiscard]] const struct CompiledGraph* debugCompiledGraph() const noexcept;

    /// @brief Test-only accessor for the frozen ExecuteSchedule. Returns nullptr before a
    ///        successful compile(). Consumers must include Internal/ExecuteSchedule.h.
    [[nodiscard]] const struct ExecuteSchedule* debugSchedule() const noexcept;

private:
    void registerPass(Rc<IRenderPass> Pass, std::string_view Name);
    void connectImpl(PassId SrcPass, FieldId SrcField, PassId DstPass, FieldId DstField,
                     RgResourceType Rt);
    void markOutputImpl(PassId Pass, FieldId Field, rhi::RhiTextureLayout Terminal);

    std::unique_ptr<Impl> P_;
};

} // namespace goleta::rg
