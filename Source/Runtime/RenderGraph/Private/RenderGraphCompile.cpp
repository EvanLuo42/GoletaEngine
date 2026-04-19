#include <algorithm>
#include <cstdint>
#include <unordered_set>

#include "AccessStateTable.h"
#include "BarrierPlanner.h"
#include "Log.h"
#include "RenderGraph.h"
#include "RenderGraphImpl.h"
#include "TopologicalSort.h"

namespace goleta::rg
{

namespace
{

struct UnionFind
{
    std::vector<uint32_t> Parent;
    std::vector<uint32_t> Rank;

    void reset(uint32_t N)
    {
        Parent.resize(N);
        Rank.assign(N, 0);
        for (uint32_t i = 0; i < N; ++i)
            Parent[i] = i;
    }

    uint32_t find(uint32_t X)
    {
        while (Parent[X] != X)
        {
            Parent[X] = Parent[Parent[X]];
            X         = Parent[X];
        }
        return X;
    }

    void unite(uint32_t A, uint32_t B)
    {
        A = find(A);
        B = find(B);
        if (A == B)
            return;
        if (Rank[A] < Rank[B])
            std::swap(A, B);
        Parent[B] = A;
        if (Rank[A] == Rank[B])
            ++Rank[A];
    }
};

struct FieldCoord
{
    PassId  Pass;
    FieldId Field;
};

uint32_t flattenFieldIndex(const RenderGraph::Impl&                    Impl,
                           const std::vector<uint32_t>&                PassFieldBase,
                           PassId Pass, FieldId Field)
{
    return PassFieldBase[static_cast<uint32_t>(Pass)] + static_cast<uint32_t>(Field);
}

void mergeTextureDesc(rhi::RhiTextureDesc& Out, const FieldDesc& F)
{
    using U = rhi::RhiTextureUsage;
    if (F.Format != rhi::RhiFormat::Unknown && Out.Format == rhi::RhiFormat::Unknown)
        Out.Format = F.Format;
    if (F.Width != 0 && Out.Width <= 1)
        Out.Width = F.Width;
    if (F.Height != 0 && Out.Height <= 1)
        Out.Height = F.Height;
    if (F.Depth != 0 && Out.DepthOrArrayLayers <= 1)
        Out.DepthOrArrayLayers = F.Depth;
    if (F.ArrayLayers > 1)
        Out.DepthOrArrayLayers = F.ArrayLayers;
    if (F.MipLevels > Out.MipLevels)
        Out.MipLevels = F.MipLevels;
    if (F.Samples != rhi::RhiSampleCount::X1)
        Out.Samples = F.Samples;
    Out.Usage = Out.Usage | F.TextureUsage;
    // Derive extra usage from BindAs if the author left the usage mask default.
    switch (F.BindAs)
    {
    case RgBindAs::Sampled:         Out.Usage = Out.Usage | U::Sampled; break;
    case RgBindAs::Storage:         Out.Usage = Out.Usage | U::Storage; break;
    case RgBindAs::ColorAttachment: Out.Usage = Out.Usage | U::ColorAttachment; break;
    case RgBindAs::DepthAttachment: Out.Usage = Out.Usage | U::DepthAttachment; break;
    case RgBindAs::CopySrc:         Out.Usage = Out.Usage | U::CopySource; break;
    case RgBindAs::CopyDst:         Out.Usage = Out.Usage | U::CopyDest; break;
    case RgBindAs::Auto:            break;
    }
    // Auto-add Sampled if any consumer reads and no explicit bits are set.
    if (F.AccessMode == RgAccessMode::Input && F.BindAs == RgBindAs::Auto)
        Out.Usage = Out.Usage | U::Sampled;
}

void mergeBufferDesc(rhi::RhiBufferDesc& Out, const FieldDesc& F)
{
    if (F.SizeBytes > Out.SizeBytes)
        Out.SizeBytes = F.SizeBytes;
    if (F.StructureStride != 0 && Out.StructureStride == 0)
        Out.StructureStride = F.StructureStride;
    Out.Usage = Out.Usage | F.BufferUsage;
}

} // namespace

Result<void, RgError> RenderGraph::compile(rhi::IRhiDevice* Device)
{
    (void)Device;

    auto& I = *P_;

    I.Compiled   = CompiledGraph{};
    I.Schedule   = ExecuteSchedule{};
    I.Compiled_  = false;

    const uint32_t PassCount = static_cast<uint32_t>(I.Passes.size());

    std::vector<uint32_t> PassFieldBase(PassCount + 1, 0);
    for (uint32_t p = 0; p < PassCount; ++p)
    {
        const uint32_t FieldCount =
            static_cast<uint32_t>(I.Reflections[p]->fields().size());
        PassFieldBase[p + 1] = PassFieldBase[p] + FieldCount;
    }
    const uint32_t TotalFields = PassFieldBase[PassCount];

    for (uint32_t p = 0; p < PassCount; ++p)
    {
        const auto& Refl = *I.Reflections[p];
        std::unordered_set<std::string> Seen;
        for (const auto& F : Refl.fields())
        {
            if (!Seen.insert(F.Name).second)
            {
                GOLETA_LOG_ERROR(RenderGraph,
                                 "Pass '{}' declares duplicate field '{}'",
                                 I.PassNames[p], F.Name);
                return Err{RgError::DuplicateField};
            }
        }
    }

    UnionFind UF;
    UF.reset(TotalFields);

    std::vector<uint32_t> EdgesFrom;
    std::vector<uint32_t> EdgesTo;
    EdgesFrom.reserve(I.Edges.size());
    EdgesTo.reserve(I.Edges.size());

    for (const auto& E : I.Edges)
    {
        const uint32_t Sp = static_cast<uint32_t>(E.SrcPass);
        const uint32_t Dp = static_cast<uint32_t>(E.DstPass);
        if (Sp >= PassCount || Dp >= PassCount)
        {
            GOLETA_LOG_ERROR(RenderGraph, "Connect references unknown pass");
            return Err{RgError::InvalidHandle};
        }
        const uint32_t Sf = static_cast<uint32_t>(E.SrcField);
        const uint32_t Df = static_cast<uint32_t>(E.DstField);
        if (Sf >= I.Reflections[Sp]->fields().size() ||
            Df >= I.Reflections[Dp]->fields().size())
        {
            GOLETA_LOG_ERROR(RenderGraph, "Connect references unknown field");
            return Err{RgError::InvalidHandle};
        }

        const auto& SrcField = I.Reflections[Sp]->fields()[Sf];
        const auto& DstField = I.Reflections[Dp]->fields()[Df];
        if (SrcField.ResourceType != DstField.ResourceType)
        {
            GOLETA_LOG_ERROR(RenderGraph, "Connect type mismatch: '{}.{}' vs '{}.{}'",
                             I.PassNames[Sp], SrcField.Name, I.PassNames[Dp], DstField.Name);
            return Err{RgError::TypeMismatch};
        }
        if (SrcField.AccessMode == RgAccessMode::Input)
        {
            GOLETA_LOG_ERROR(RenderGraph, "Connect source '{}.{}' is declared as Input",
                             I.PassNames[Sp], SrcField.Name);
            return Err{RgError::DirectionMismatch};
        }
        if (DstField.AccessMode == RgAccessMode::Output)
        {
            GOLETA_LOG_ERROR(RenderGraph, "Connect destination '{}.{}' is declared as Output",
                             I.PassNames[Dp], DstField.Name);
            return Err{RgError::DirectionMismatch};
        }

        UF.unite(flattenFieldIndex(I, PassFieldBase, E.SrcPass, E.SrcField),
                 flattenFieldIndex(I, PassFieldBase, E.DstPass, E.DstField));

        EdgesFrom.push_back(Sp);
        EdgesTo.push_back(Dp);
    }

    std::vector<uint32_t> Imported(TotalFields, UINT32_MAX);
    for (uint32_t i = 0; i < I.Imports.size(); ++i)
    {
        const auto&  B  = I.Imports[i];
        auto It = I.PassByName.find(B.PassName);
        if (It == I.PassByName.end())
        {
            GOLETA_LOG_ERROR(RenderGraph, "setInput: unknown pass '{}'", B.PassName);
            return Err{RgError::InvalidHandle};
        }
        const uint32_t Pi = static_cast<uint32_t>(It->second);
        const FieldId  Fi = I.Reflections[Pi]->findByName(B.FieldName);
        if (Fi == FieldId::Invalid)
        {
            GOLETA_LOG_ERROR(RenderGraph, "setInput: unknown field '{}' on pass '{}'",
                             B.FieldName, B.PassName);
            return Err{RgError::InvalidHandle};
        }
        Imported[flattenFieldIndex(I, PassFieldBase, It->second, Fi)] = i;
    }

    std::vector<uint32_t> RootToLogical(TotalFields, UINT32_MAX);
    std::vector<LogicalResourceInfo> Logicals;
    Logicals.reserve(TotalFields);

    I.Compiled.FieldToLogical.assign(PassCount, {});
    for (uint32_t p = 0; p < PassCount; ++p)
        I.Compiled.FieldToLogical[p].assign(I.Reflections[p]->fields().size(),
                                            LogicalResourceId::Invalid);

    for (uint32_t p = 0; p < PassCount; ++p)
    {
        const auto& Refl = *I.Reflections[p];
        for (uint32_t f = 0; f < Refl.fields().size(); ++f)
        {
            const uint32_t Idx  = PassFieldBase[p] + f;
            const uint32_t Root = UF.find(Idx);
            uint32_t       Lid  = RootToLogical[Root];
            if (Lid == UINT32_MAX)
            {
                LogicalResourceInfo Info;
                Info.ResourceType = Refl.fields()[f].ResourceType;
                Info.Lifetime     = Refl.fields()[f].Lifetime;
                if (Info.ResourceType == RgResourceType::Texture)
                {
                    Info.TextureDesc                    = rhi::RhiTextureDesc{};
                    Info.TextureDesc.Format             = rhi::RhiFormat::Unknown;
                    Info.TextureDesc.Width              = 0;
                    Info.TextureDesc.Height             = 0;
                    Info.TextureDesc.DepthOrArrayLayers = 0;
                    Info.TextureDesc.Usage              = rhi::RhiTextureUsage::None;
                }
                else
                {
                    Info.BufferDesc = rhi::RhiBufferDesc{};
                }
                Lid = static_cast<uint32_t>(Logicals.size());
                Logicals.emplace_back(std::move(Info));
                RootToLogical[Root] = Lid;
            }
            I.Compiled.FieldToLogical[p][f] = static_cast<LogicalResourceId>(Lid);

            auto& L = Logicals[Lid];
            const auto& F = Refl.fields()[f];

            if (L.DebugName.empty())
                L.DebugName = I.PassNames[p] + "." + F.Name;

            if (F.Lifetime == RgLifetime::Persistent)
                L.Lifetime = RgLifetime::Persistent;

            if (L.ResourceType == RgResourceType::Texture)
                mergeTextureDesc(L.TextureDesc, F);
            else
                mergeBufferDesc(L.BufferDesc, F);

            if (Imported[Idx] != UINT32_MAX)
            {
                const auto& B = I.Imports[Imported[Idx]];
                L.Lifetime      = RgLifetime::Imported;
                L.InitialLayout = B.InitialLayout;
                L.InitialAccess = B.InitialAccess;
                if (L.ResourceType == RgResourceType::Texture)
                    L.ImportedTexture = B.Texture;
                else
                    L.ImportedBuffer = B.Buffer;
            }
        }
    }

    // Unresolved input check: an Input that isn't wired and isn't imported is an error.
    for (uint32_t p = 0; p < PassCount; ++p)
    {
        const auto& Refl = *I.Reflections[p];
        for (uint32_t f = 0; f < Refl.fields().size(); ++f)
        {
            const auto& F = Refl.fields()[f];
            if (F.AccessMode != RgAccessMode::Input)
                continue;
            const uint32_t Idx = PassFieldBase[p] + f;
            const uint32_t Lid = RootToLogical[UF.find(Idx)];
            const auto& L      = Logicals[Lid];
            if (L.Lifetime == RgLifetime::Imported)
                continue;
            const bool HasProducer =
                std::any_of(I.Edges.begin(), I.Edges.end(), [&](const GraphEdge& E) {
                    return E.DstPass == static_cast<PassId>(p) &&
                           E.DstField == static_cast<FieldId>(f);
                });
            if (!HasProducer)
            {
                GOLETA_LOG_ERROR(RenderGraph, "Unresolved input '{}.{}'",
                                 I.PassNames[p], F.Name);
                return Err{RgError::UnresolvedInput};
            }
        }
    }

    // Validate that texture outputs have enough metadata (format + extent) post-merge.
    for (uint32_t p = 0; p < PassCount; ++p)
    {
        const auto& Refl = *I.Reflections[p];
        for (uint32_t f = 0; f < Refl.fields().size(); ++f)
        {
            const auto& F = Refl.fields()[f];
            if (F.AccessMode == RgAccessMode::Input)
                continue;
            const uint32_t Lid = RootToLogical[UF.find(PassFieldBase[p] + f)];
            const auto& L      = Logicals[Lid];
            if (L.Lifetime == RgLifetime::Imported)
                continue;
            if (L.ResourceType == RgResourceType::Texture)
            {
                if (L.TextureDesc.Format == rhi::RhiFormat::Unknown ||
                    L.TextureDesc.Width == 0 || L.TextureDesc.Height == 0)
                {
                    GOLETA_LOG_ERROR(RenderGraph,
                                     "Texture output '{}.{}' missing format/extent",
                                     I.PassNames[p], F.Name);
                    return Err{RgError::MissingMetadata};
                }
            }
            else
            {
                if (L.BufferDesc.SizeBytes == 0)
                {
                    GOLETA_LOG_ERROR(RenderGraph,
                                     "Buffer output '{}.{}' missing size",
                                     I.PassNames[p], F.Name);
                    return Err{RgError::MissingMetadata};
                }
            }
        }
    }

    // Queue-aware Kahn's: among ready passes, prefer one whose queue kind matches the most
    // recently scheduled pass. Keeps same-queue passes adjacent so the group merger folds
    // them into a single command list.
    std::vector<uint32_t> Order;
    {
        std::vector<uint32_t> InDegree(PassCount, 0);
        std::vector<std::vector<uint32_t>> Adj(PassCount);
        for (size_t i = 0; i < EdgesFrom.size(); ++i)
        {
            Adj[EdgesFrom[i]].push_back(EdgesTo[i]);
            ++InDegree[EdgesTo[i]];
        }
        std::vector<bool> Ready(PassCount, false);
        for (uint32_t n = 0; n < PassCount; ++n)
            if (InDegree[n] == 0)
                Ready[n] = true;

        rhi::RhiQueueKind LastQueue = rhi::RhiQueueKind::Graphics;
        Order.reserve(PassCount);
        while (Order.size() < PassCount)
        {
            int32_t Pick = -1;
            for (uint32_t n = 0; n < PassCount; ++n)
                if (Ready[n] && I.Passes[n]->preferredQueue() == LastQueue)
                {
                    Pick = static_cast<int32_t>(n);
                    break;
                }
            if (Pick < 0)
                for (uint32_t n = 0; n < PassCount; ++n)
                    if (Ready[n])
                    {
                        Pick = static_cast<int32_t>(n);
                        break;
                    }
            if (Pick < 0)
            {
                GOLETA_LOG_ERROR(RenderGraph, "RenderGraph '{}' contains a cycle", I.DebugName);
                return Err{RgError::Cycle};
            }
            Order.push_back(static_cast<uint32_t>(Pick));
            Ready[Pick] = false;
            LastQueue   = I.Passes[Pick]->preferredQueue();
            for (uint32_t m : Adj[Pick])
                if (--InDegree[m] == 0)
                    Ready[m] = true;
        }
    }

    I.Compiled.LogicalResources = std::move(Logicals);
    I.Compiled.PassOrder.resize(PassCount);
    for (uint32_t i = 0; i < PassCount; ++i)
        I.Compiled.PassOrder[i] = static_cast<PassId>(Order[i]);

    std::vector<PlannerPassView> PlannerPasses;
    PlannerPasses.reserve(PassCount);
    for (uint32_t i = 0; i < PassCount; ++i)
    {
        const uint32_t Pid = Order[i];
        PlannerPasses.push_back(PlannerPassView{static_cast<PassId>(Pid),
                                                I.Reflections[Pid].get(),
                                                I.Passes[Pid]->preferredQueue()});
    }

    std::vector<PlannerMarkedOutput> PlannerMarks;
    PlannerMarks.reserve(I.MarkedOutputs.size());
    for (const auto& M : I.MarkedOutputs)
    {
        const uint32_t Pi = static_cast<uint32_t>(M.Pass);
        const uint32_t Fi = static_cast<uint32_t>(M.Field);
        if (Pi >= PassCount || Fi >= I.Reflections[Pi]->fields().size())
        {
            GOLETA_LOG_ERROR(RenderGraph, "markOutput: invalid handle");
            return Err{RgError::InvalidHandle};
        }
        const LogicalResourceId Lid = I.Compiled.FieldToLogical[Pi][Fi];
        PlannerMarks.push_back(PlannerMarkedOutput{Lid, M.Terminal});
    }

    const rhi::RhiQueueKind FinalQueue =
        PassCount == 0
            ? rhi::RhiQueueKind::Graphics
            : I.Passes[static_cast<uint32_t>(PlannerPasses.back().Id)]->preferredQueue();
    planBarriers(PlannerPasses, I.Compiled.LogicalResources, I.Compiled.FieldToLogical,
                 PlannerMarks, FinalQueue, I.Compiled.PrePassBarriers, I.Compiled.FinalBarriers);

    // Lifetime ranges for transient resources (not used directly in phase 1 but populated for
    // future aliasing work).
    for (auto& R : I.Compiled.LogicalResources)
    {
        R.FirstUsePassIdx = UINT32_MAX;
        R.LastUsePassIdx  = 0;
    }
    for (uint32_t i = 0; i < PassCount; ++i)
    {
        const uint32_t Pid = Order[i];
        const auto&    Row = I.Compiled.FieldToLogical[Pid];
        for (LogicalResourceId Id : Row)
        {
            if (Id == LogicalResourceId::Invalid)
                continue;
            auto& R = I.Compiled.LogicalResources[static_cast<uint32_t>(Id)];
            if (R.FirstUsePassIdx == UINT32_MAX)
                R.FirstUsePassIdx = i;
            R.LastUsePassIdx = i;
        }
    }

    I.Schedule.Groups.clear();
    I.Schedule.FenceValuesPerFrame = 0;

    // Group adjacent same-queue passes together; cross-queue transitions start a new group.
    // Each pass's preferred queue is resolved from its IRenderPass::preferredQueue() override,
    // with fallback to Graphics when the device lacks a matching queue (checked at execute()).
    std::vector<uint32_t> PassIndexToGroup(PassCount, UINT32_MAX);
    for (uint32_t i = 0; i < PassCount; ++i)
    {
        const uint32_t      Pid   = Order[i];
        const rhi::RhiQueueKind Q = I.Passes[Pid]->preferredQueue();
        if (I.Schedule.Groups.empty() || I.Schedule.Groups.back().QueueKind != Q)
        {
            ScheduleGroup NewG;
            NewG.QueueKind    = Q;
            NewG.SignalOffset = static_cast<uint64_t>(I.Schedule.Groups.size()) + 1ull;
            I.Schedule.Groups.push_back(std::move(NewG));
        }
        const uint32_t GroupIdx = static_cast<uint32_t>(I.Schedule.Groups.size()) - 1;
        I.Schedule.Groups.back().Passes.push_back(i);
        PassIndexToGroup[Pid] = GroupIdx;
    }
    I.Schedule.FenceValuesPerFrame = static_cast<uint64_t>(I.Schedule.Groups.size());

    // Compute per-group cross-queue wait dependencies. For each edge producer→consumer, if the
    // producer's group is on a different queue than the consumer's group, the consumer group
    // must wait on the producer group's fence signal.
    for (const auto& E : I.Edges)
    {
        const uint32_t Sp = static_cast<uint32_t>(E.SrcPass);
        const uint32_t Dp = static_cast<uint32_t>(E.DstPass);
        const uint32_t Sg = PassIndexToGroup[Sp];
        const uint32_t Dg = PassIndexToGroup[Dp];
        if (Sg == Dg || Sg == UINT32_MAX || Dg == UINT32_MAX)
            continue;
        if (I.Schedule.Groups[Sg].QueueKind == I.Schedule.Groups[Dg].QueueKind)
            continue; // Same queue is implicitly ordered by submission.

        auto& Waits = I.Schedule.Groups[Dg].WaitGroups;
        if (std::find(Waits.begin(), Waits.end(), Sg) == Waits.end())
            Waits.push_back(Sg);
    }

    // Populate per-group lifetime lists so execute() can acquire at first use and release
    // after last use, letting disjoint-lifetime transients share a pooled physical resource
    // within one frame.
    {
        std::vector<uint32_t> PassToGroup(PassCount, UINT32_MAX);
        for (uint32_t g = 0; g < I.Schedule.Groups.size(); ++g)
            for (uint32_t OrderIdx : I.Schedule.Groups[g].Passes)
                PassToGroup[OrderIdx] = g;

        for (uint32_t lid = 0; lid < I.Compiled.LogicalResources.size(); ++lid)
        {
            const auto& R = I.Compiled.LogicalResources[lid];
            if (R.Lifetime != RgLifetime::Transient)
                continue;
            const uint32_t FirstG = PassToGroup[R.FirstUsePassIdx];
            const uint32_t LastG  = PassToGroup[R.LastUsePassIdx];
            if (FirstG >= I.Schedule.Groups.size() || LastG >= I.Schedule.Groups.size())
                continue;
            I.Schedule.Groups[FirstG].AcquireAtStart.push_back(lid);
            I.Schedule.Groups[LastG].ReleaseAfter.push_back(lid);
        }
    }

    I.Dirty     = false;
    I.Compiled_ = true;
    return {};
}

} // namespace goleta::rg
