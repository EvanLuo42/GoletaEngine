#include "RenderGraph.h"

#include <cassert>
#include <utility>

#include "Log.h"
#include "RenderGraphImpl.h"

namespace goleta::rg
{

RenderGraph::RenderGraph(std::string DebugName)
    : P_(std::make_unique<Impl>())
{
    P_->DebugName = std::move(DebugName);
}

RenderGraph::~RenderGraph() = default;

void RenderGraph::registerPass(Rc<IRenderPass> Pass, std::string_view Name)
{
    const uint32_t NewIdx = static_cast<uint32_t>(P_->Passes.size());
    const PassId   Id     = static_cast<PassId>(NewIdx);

    Pass->Name_ = std::string(Name);
    Pass->Id_   = Id;

    P_->PassNames.emplace_back(Name);
    P_->PassByName.emplace(std::string(Name), Id);

    std::unique_ptr<RenderPassReflection> Refl{new RenderPassReflection(Id)};
    Pass->reflect(*Refl);

    P_->Reflections.emplace_back(std::move(Refl));
    P_->Passes.emplace_back(std::move(Pass));

    P_->Dirty = true;
    P_->Compiled_ = false;
}

void RenderGraph::connectImpl(PassId SrcPass, FieldId SrcField, PassId DstPass, FieldId DstField,
                              RgResourceType /*Rt*/)
{
    P_->Edges.push_back(GraphEdge{SrcPass, SrcField, DstPass, DstField});
    P_->Dirty = true;
    P_->Compiled_ = false;
}

void RenderGraph::markOutputImpl(PassId Pass, FieldId Field, rhi::RhiTextureLayout Terminal)
{
    P_->MarkedOutputs.push_back(MarkedOutputRequest{Pass, Field, Terminal});
    P_->Dirty = true;
    P_->Compiled_ = false;
}

void RenderGraph::setInput(std::string_view PassName, std::string_view FieldName,
                           Rc<rhi::IRhiTexture> Imported, rhi::RhiTextureLayout InitialLayout,
                           rhi::RhiAccess InitialAccess)
{
    ImportedBinding B;
    B.PassName      = std::string(PassName);
    B.FieldName     = std::string(FieldName);
    B.Texture       = std::move(Imported);
    B.InitialLayout = InitialLayout;
    B.InitialAccess = InitialAccess;
    P_->Imports.push_back(std::move(B));
    P_->Dirty     = true;
    P_->Compiled_ = false;
}

void RenderGraph::setInput(std::string_view PassName, std::string_view FieldName,
                           Rc<rhi::IRhiBuffer> Imported)
{
    ImportedBinding B;
    B.PassName  = std::string(PassName);
    B.FieldName = std::string(FieldName);
    B.Buffer    = std::move(Imported);
    P_->Imports.push_back(std::move(B));
    P_->Dirty     = true;
    P_->Compiled_ = false;
}

std::span<const std::string> RenderGraph::passNames() const noexcept
{
    return {P_->PassNames.data(), P_->PassNames.size()};
}

bool RenderGraph::isCompiled() const noexcept
{
    return P_->Compiled_ && !P_->Dirty;
}

const std::string& RenderGraph::debugName() const noexcept
{
    return P_->DebugName;
}

const CompiledGraph* RenderGraph::debugCompiledGraph() const noexcept
{
    if (!P_->Compiled_)
        return nullptr;
    return &P_->Compiled;
}

const ExecuteSchedule* RenderGraph::debugSchedule() const noexcept
{
    if (!P_->Compiled_)
        return nullptr;
    return &P_->Schedule;
}

void RenderGraph::setTaskScheduler(void* EnkiTaskScheduler) noexcept
{
    P_->TaskScheduler = static_cast<enki::TaskScheduler*>(EnkiTaskScheduler);
}

void RenderGraph::captureNextFrame(std::string Name) noexcept
{
    P_->CaptureRequested = true;
    P_->CaptureName      = std::move(Name);
}

void RenderGraph::setTimingEnabled(bool On) noexcept
{
    P_->TimingsEnabled = On;
    if (!On)
    {
        P_->TimingsLastFrame.clear();
        P_->TimingsInFlight.clear();
    }
}

std::span<const RenderGraph::PassTiming> RenderGraph::lastFrameTimings() const noexcept
{
    return {P_->TimingsLastFrame.data(), P_->TimingsLastFrame.size()};
}

} // namespace goleta::rg
