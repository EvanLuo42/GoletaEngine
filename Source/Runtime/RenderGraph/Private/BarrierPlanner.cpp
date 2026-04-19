#include "BarrierPlanner.h"

#include <cstdint>

#include "AccessStateTable.h"
#include "RHIBuffer.h"
#include "RHITexture.h"
#include "RenderPassReflection.h"

namespace goleta::rg
{

namespace
{

struct TrackedTextureState
{
    rhi::RhiTextureLayout Layout     = rhi::RhiTextureLayout::Undefined;
    rhi::RhiAccess        Access     = rhi::RhiAccess::None;
    rhi::RhiPipelineStage Stages     = rhi::RhiPipelineStage::None;
    rhi::RhiQueueKind     LastQueue  = rhi::RhiQueueKind::Graphics;
    bool                  FirstTouch = true;
};

struct TrackedBufferState
{
    rhi::RhiAccess        Access     = rhi::RhiAccess::None;
    rhi::RhiPipelineStage Stages     = rhi::RhiPipelineStage::None;
    rhi::RhiQueueKind     LastQueue  = rhi::RhiQueueKind::Graphics;
    bool                  FirstTouch = true;
};

/// @brief Pipeline stages that are legal to reference on a given queue family's command list.
///        Stages recorded in a barrier on a compute cmd list must be a subset of this mask;
///        stages produced by a different queue are represented as SYNC_NONE with a timeline
///        fence handling the actual ordering outside the barrier.
rhi::RhiPipelineStage queueCompatibleStages(rhi::RhiQueueKind Queue) noexcept
{
    using P = rhi::RhiPipelineStage;
    switch (Queue)
    {
    case rhi::RhiQueueKind::Graphics:
        return P::DrawIndirect | P::VertexInput | P::VertexShader | P::PixelShader |
               P::EarlyFragmentTests | P::LateFragmentTests | P::ColorAttachmentOut |
               P::ComputeShader | P::RayTracingShader | P::Copy | P::Resolve |
               P::AccelerationBuild | P::AllGraphics | P::AllCommands;
    case rhi::RhiQueueKind::Compute:
        return P::DrawIndirect | P::ComputeShader | P::RayTracingShader | P::Copy |
               P::AccelerationBuild | P::AllCommands;
    case rhi::RhiQueueKind::Copy:
        return P::Copy | P::AllCommands;
    case rhi::RhiQueueKind::Video:
    case rhi::RhiQueueKind::Count:
    default:
        return P::AllCommands;
    }
}

rhi::RhiPipelineStage maskStages(rhi::RhiPipelineStage Stages,
                                 rhi::RhiQueueKind     Queue) noexcept
{
    const rhi::RhiPipelineStage Allowed = queueCompatibleStages(Queue);
    const rhi::RhiPipelineStage Masked  = Stages & Allowed;
    return Masked == rhi::RhiPipelineStage::None ? rhi::RhiPipelineStage::None : Masked;
}

} // namespace

void planBarriers(std::span<const PlannerPassView>                   Passes,
                  std::span<const LogicalResourceInfo>               LogicalResources,
                  const std::vector<std::vector<LogicalResourceId>>& FieldToLogical,
                  std::span<const PlannerMarkedOutput>               MarkedOutputs,
                  rhi::RhiQueueKind                                  FinalQueue,
                  std::vector<PassBarriers>&                         OutPrePassBarriers,
                  PassBarriers&                                      OutFinalBarriers)
{
    std::vector<TrackedTextureState> TexStates(LogicalResources.size());
    std::vector<TrackedBufferState>  BufStates(LogicalResources.size());

    for (size_t i = 0; i < LogicalResources.size(); ++i)
    {
        const auto& R = LogicalResources[i];
        if (R.Lifetime != RgLifetime::Imported)
            continue;
        if (R.ResourceType == RgResourceType::Texture)
        {
            TexStates[i].Layout     = R.InitialLayout;
            TexStates[i].Access     = R.InitialAccess;
            TexStates[i].FirstTouch = R.InitialLayout == rhi::RhiTextureLayout::Undefined &&
                                      R.InitialAccess == rhi::RhiAccess::None;
        }
        else
        {
            BufStates[i].Access     = R.InitialAccess;
            BufStates[i].FirstTouch = R.InitialAccess == rhi::RhiAccess::None;
        }
    }

    OutPrePassBarriers.assign(Passes.size(), PassBarriers{});

    for (size_t Pi = 0; Pi < Passes.size(); ++Pi)
    {
        const auto& PV = Passes[Pi];
        if (PV.Reflection == nullptr)
            continue;

        PassBarriers& OutBarriers = OutPrePassBarriers[Pi];
        const auto&   FieldsVec   = FieldToLogical[static_cast<uint32_t>(PV.Id)];

        for (size_t Fi = 0; Fi < PV.Reflection->fields().size(); ++Fi)
        {
            const LogicalResourceId LidEnum = FieldsVec[Fi];
            if (LidEnum == LogicalResourceId::Invalid)
                continue;
            const uint32_t Lid = static_cast<uint32_t>(LidEnum);
            const auto&    R   = LogicalResources[Lid];
            const auto&    F   = PV.Reflection->fields()[Fi];

            if (R.ResourceType == RgResourceType::Texture)
            {
                const RgBindAs Bind = detail::inferTextureBind(F.BindAs, R.TextureDesc.Usage);
                const auto     Want = detail::textureStateFor(Bind, F.AccessMode);

                auto&      S        = TexStates[Lid];
                const bool CrossQueue = !S.FirstTouch && S.LastQueue != PV.QueueKind;
                const bool NeedBarrier =
                    S.FirstTouch || S.Layout != Want.Layout || S.Access != Want.Access ||
                    Want.Writes;

                if (NeedBarrier)
                {
                    rhi::RhiTextureBarrier B{};
                    // On cross-queue boundaries the timeline fence orders the work, so the
                    // barrier's Src* are zeroed to keep it queue-compatible.
                    const rhi::RhiPipelineStage RawSrcStages =
                        S.FirstTouch ? rhi::RhiPipelineStage::AllCommands : S.Stages;
                    B.SrcStages = (CrossQueue || S.FirstTouch)
                                      ? rhi::RhiPipelineStage::None
                                      : maskStages(RawSrcStages, PV.QueueKind);
                    B.DstStages = maskStages(Want.Stages, PV.QueueKind);
                    B.SrcAccess = CrossQueue || S.FirstTouch ? rhi::RhiAccess::None : S.Access;
                    B.DstAccess = Want.Access;
                    B.SrcLayout = S.FirstTouch ? rhi::RhiTextureLayout::Undefined : S.Layout;
                    B.DstLayout = Want.Layout;
                    B.BaseMipLevel    = 0;
                    B.MipLevelCount   = R.TextureDesc.MipLevels;
                    B.BaseArrayLayer  = 0;
                    B.ArrayLayerCount = R.TextureDesc.DepthOrArrayLayers;
                    B.DiscardContents =
                        S.FirstTouch && F.AccessMode == RgAccessMode::Output;
                    OutBarriers.Textures.push_back(B);
                    OutBarriers.TextureIds.push_back(LidEnum);
                }

                S.Layout     = Want.Layout;
                S.Access     = Want.Access;
                S.Stages     = Want.Stages;
                S.LastQueue  = PV.QueueKind;
                S.FirstTouch = false;
            }
            else
            {
                const auto Want =
                    detail::bufferStateFor(F.BindAs, F.AccessMode, R.BufferDesc.Usage);

                auto&      S          = BufStates[Lid];
                const bool CrossQueue = !S.FirstTouch && S.LastQueue != PV.QueueKind;
                const bool NeedBarrier = S.FirstTouch || S.Access != Want.Access || Want.Writes;

                if (NeedBarrier)
                {
                    rhi::RhiBufferBarrier B{};
                    const rhi::RhiPipelineStage RawSrcStages =
                        S.FirstTouch ? rhi::RhiPipelineStage::AllCommands : S.Stages;
                    B.SrcStages = (CrossQueue || S.FirstTouch)
                                      ? rhi::RhiPipelineStage::None
                                      : maskStages(RawSrcStages, PV.QueueKind);
                    B.DstStages = maskStages(Want.Stages, PV.QueueKind);
                    B.SrcAccess = CrossQueue || S.FirstTouch ? rhi::RhiAccess::None : S.Access;
                    B.DstAccess = Want.Access;
                    B.Offset    = 0;
                    B.Size      = ~uint64_t{0};
                    OutBarriers.Buffers.push_back(B);
                    OutBarriers.BufferIds.push_back(LidEnum);
                }

                S.Access     = Want.Access;
                S.Stages     = Want.Stages;
                S.LastQueue  = PV.QueueKind;
                S.FirstTouch = false;
            }
        }
    }

    for (const auto& M : MarkedOutputs)
    {
        if (M.Resource == LogicalResourceId::Invalid)
            continue;
        const uint32_t Lid = static_cast<uint32_t>(M.Resource);
        if (LogicalResources[Lid].ResourceType != RgResourceType::Texture)
            continue;

        auto& S = TexStates[Lid];
        if (!S.FirstTouch && S.Layout == M.Terminal)
            continue;

        rhi::RhiTextureBarrier B{};
        const bool CrossQueue = !S.FirstTouch && S.LastQueue != FinalQueue;
        const rhi::RhiPipelineStage RawSrcStages =
            S.FirstTouch ? rhi::RhiPipelineStage::AllCommands : S.Stages;
        B.SrcStages = (CrossQueue || S.FirstTouch)
                          ? rhi::RhiPipelineStage::None
                          : maskStages(RawSrcStages, FinalQueue);
        B.DstStages = maskStages(rhi::RhiPipelineStage::AllCommands, FinalQueue);
        B.SrcAccess = CrossQueue || S.FirstTouch ? rhi::RhiAccess::None : S.Access;
        B.DstAccess = M.Terminal == rhi::RhiTextureLayout::Present
                          ? rhi::RhiAccess::Present
                          : rhi::RhiAccess::ShaderResourceRead;
        B.SrcLayout       = S.FirstTouch ? rhi::RhiTextureLayout::Undefined : S.Layout;
        B.DstLayout       = M.Terminal;
        B.BaseMipLevel    = 0;
        B.MipLevelCount   = LogicalResources[Lid].TextureDesc.MipLevels;
        B.BaseArrayLayer  = 0;
        B.ArrayLayerCount = LogicalResources[Lid].TextureDesc.DepthOrArrayLayers;
        OutFinalBarriers.Textures.push_back(B);
        OutFinalBarriers.TextureIds.push_back(M.Resource);

        S.Layout     = M.Terminal;
        S.Access     = B.DstAccess;
        S.Stages     = B.DstStages;
        S.FirstTouch = false;
    }
}

} // namespace goleta::rg
