#include <cassert>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <vector>

#if GOLETA_RG_HAS_ENKITS
#include "TaskScheduler.h"
#endif

#include "CompiledGraph.h"
#include "Log.h"
#include "RHICommandList.h"
#include "RHIDebug.h"
#include "RHIDevice.h"
#include "RHIQueue.h"
#include "RHISync.h"
#include "RenderContext.h"
#include "RenderData.h"
#include "RenderGraph.h"
#include "RenderGraphImpl.h"

namespace goleta::rg
{

namespace
{

void patchTextureBarriers(const std::vector<rhi::RhiTextureBarrier>& Src,
                          const std::vector<LogicalResourceId>&      Ids,
                          const ResourceRegistry&                    Registry,
                          std::vector<rhi::RhiTextureBarrier>&       Out)
{
    Out.clear();
    Out.reserve(Src.size());
    for (size_t i = 0; i < Src.size(); ++i)
    {
        rhi::RhiTextureBarrier B = Src[i];
        B.Texture                = Registry.texture(Ids[i]);
        if (B.Texture != nullptr)
            Out.push_back(B);
    }
}

void patchBufferBarriers(const std::vector<rhi::RhiBufferBarrier>& Src,
                         const std::vector<LogicalResourceId>&     Ids,
                         const ResourceRegistry&                   Registry,
                         std::vector<rhi::RhiBufferBarrier>&       Out)
{
    Out.clear();
    Out.reserve(Src.size());
    for (size_t i = 0; i < Src.size(); ++i)
    {
        rhi::RhiBufferBarrier B = Src[i];
        B.Buffer                = Registry.buffer(Ids[i]);
        if (B.Buffer != nullptr)
            Out.push_back(B);
    }
}

struct RecordedGroup
{
    Rc<rhi::IRhiCommandList> CmdList;
    rhi::RhiQueueKind        Queue = rhi::RhiQueueKind::Graphics;
};

uint64_t nowNs() noexcept
{
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::steady_clock::now().time_since_epoch())
            .count());
}

void recordGroup(RenderGraph::Impl&   I,
                 RenderContext&       Ctx,
                 rhi::IRhiCommandList* Cl,
                 const ScheduleGroup& G,
                 bool                 IsLastGroup,
                 std::vector<rhi::RhiTextureBarrier>& TexScratch,
                 std::vector<rhi::RhiBufferBarrier>&  BufScratch)
{
    Cl->begin();

    for (const uint32_t OrderIdx : G.Passes)
    {
        const PassId Pid     = I.Compiled.PassOrder[OrderIdx];
        const auto&  Pre     = I.Compiled.PrePassBarriers[OrderIdx];
        patchTextureBarriers(Pre.Textures, Pre.TextureIds, I.Registry, TexScratch);
        patchBufferBarriers(Pre.Buffers, Pre.BufferIds, I.Registry, BufScratch);
        if (!TexScratch.empty() || !BufScratch.empty())
        {
            rhi::RhiBarrierGroup Group;
            Group.Textures     = TexScratch.data();
            Group.TextureCount = static_cast<uint32_t>(TexScratch.size());
            Group.Buffers      = BufScratch.data();
            Group.BufferCount  = static_cast<uint32_t>(BufScratch.size());
            Cl->barriers(Group);
        }

        auto*          Pass  = I.Passes[static_cast<uint32_t>(Pid)].get();
        const uint32_t PidI  = static_cast<uint32_t>(Pid);
        const bool     Time  = I.TimingsEnabled && I.TimestampHeap &&
                           PidI * 2 + 1 < I.TimestampCapacity;
        if (Time)
            Cl->writeTimestamp(I.TimestampHeap.get(), PidI * 2);

        Cl->beginDebugScope(Pass->debugName(), 0xFF808080u);
        RenderData Rd;
        Rd.bind(&I.Compiled, &I.Registry, Pid);
        Ctx.setCmdList(Cl);

        const uint64_t CpuStart = Time ? nowNs() : 0;
        Pass->execute(Ctx, Rd);
        const uint64_t CpuEnd   = Time ? nowNs() : 0;

        Cl->endDebugScope();

        if (Time)
        {
            Cl->writeTimestamp(I.TimestampHeap.get(), PidI * 2 + 1);
            auto& T     = I.TimingsInFlight[PidI];
            T.Pass      = Pid;
            T.Name      = Pass->debugName();
            T.CpuStartNs = CpuStart;
            T.CpuEndNs   = CpuEnd;
        }
    }

    if (IsLastGroup)
    {
        const auto& Fin = I.Compiled.FinalBarriers;
        patchTextureBarriers(Fin.Textures, Fin.TextureIds, I.Registry, TexScratch);
        patchBufferBarriers(Fin.Buffers, Fin.BufferIds, I.Registry, BufScratch);
        if (!TexScratch.empty() || !BufScratch.empty())
        {
            rhi::RhiBarrierGroup Group;
            Group.Textures     = TexScratch.data();
            Group.TextureCount = static_cast<uint32_t>(TexScratch.size());
            Group.Buffers      = BufScratch.data();
            Group.BufferCount  = static_cast<uint32_t>(BufScratch.size());
            Cl->barriers(Group);
        }

        if (I.TimingsEnabled && I.TimestampHeap && I.TimestampReadback &&
            I.TimestampCapacity > 0)
        {
            Cl->resolveQueries(I.TimestampHeap.get(), 0, I.TimestampCapacity,
                               I.TimestampReadback.get(), 0);
        }
    }

    Cl->end();
}

} // namespace

void RenderGraph::execute(RenderContext& Ctx)
{
    auto& I = *P_;
    assert(I.Compiled_ && !I.Dirty && "RenderGraph::execute() requires a successful compile()");
    rhi::IRhiDevice* Device = Ctx.device();
    assert(Device != nullptr && "RenderContext missing device");

    I.Registry.beginFrame(*Device, I.Compiled, I.Pool);

    // Timing setup: (re)allocate query heap + readback when enabled or pass count changes.
    if (I.TimingsEnabled)
    {
        const uint32_t PassCount = static_cast<uint32_t>(I.Passes.size());
        const uint32_t Needed    = PassCount * 2;
        if (Needed != I.TimestampCapacity)
        {
            rhi::RhiQueryHeapDesc Qd{};
            Qd.Kind  = rhi::RhiQueryKind::Timestamp;
            Qd.Count = Needed;
            I.TimestampHeap = Device->createQueryHeap(Qd);

            rhi::RhiBufferDesc Bd{};
            Bd.SizeBytes = static_cast<uint64_t>(Needed) * sizeof(uint64_t);
            Bd.Usage     = rhi::RhiBufferUsage::CopyDest;
            Bd.Location  = rhi::RhiMemoryLocation::Readback;
            Bd.DebugName = "rg.timestamps.readback";
            I.TimestampReadback       = Device->createBuffer(Bd);
            I.TimestampCapacity       = Needed;
            I.PendingResolveFenceValue = 0;
            I.TimingsInFlight.assign(PassCount, RenderGraph::PassTiming{});
        }
        else
        {
            I.TimingsInFlight.assign(PassCount, RenderGraph::PassTiming{});
        }

        // Readback the previous frame's ticks if their resolve has finished on the GPU.
        if (I.PendingResolveFenceValue != 0 && I.TimelineFence &&
            I.TimelineFence->completedValue() >= I.PendingResolveFenceValue &&
            I.TimestampReadback && !I.TimingsLastFrame.empty())
        {
            const size_t Bytes = I.TimingsLastFrame.size() * 2 * sizeof(uint64_t);
            if (void* P = I.TimestampReadback->map(0, Bytes))
            {
                std::vector<uint64_t> Ticks(I.TimingsLastFrame.size() * 2);
                std::memcpy(Ticks.data(), P, Bytes);
                I.TimestampReadback->unmap();
                for (size_t i = 0; i < I.TimingsLastFrame.size(); ++i)
                {
                    I.TimingsLastFrame[i].GpuStartTick = Ticks[i * 2];
                    I.TimingsLastFrame[i].GpuEndTick   = Ticks[i * 2 + 1];
                }
            }
            I.PendingResolveFenceValue = 0;
        }
    }

    if (!I.TimelineFence)
    {
        I.TimelineFence  = Device->createFence(0);
        I.FenceFrameBase = 0;
    }

    // Reset any pools whose prior work has finished. The previous frame's last signal value
    // is FenceFrameBase; if it has been reached, every pool touched last frame is safe to reset.
    if (I.FenceFrameBase != 0 && I.TimelineFence)
    {
        const uint64_t Completed = I.TimelineFence->completedValue();
        if (Completed >= I.FenceFrameBase)
            I.CommandPools.resetAll();
    }

    const uint64_t FrameBase = I.FenceFrameBase;

    // Phase 2: record serially on the main thread (optionally parallelise group recording
    // via enkiTS while keeping submission single-threaded). Submits happen in schedule order
    // on the main thread, using timeline-fence values for cross-queue sync.
    std::vector<RecordedGroup> Recorded(I.Schedule.Groups.size());

    auto recordOne = [&](uint32_t Gi) {
        const auto& G = I.Schedule.Groups[Gi];
        I.Registry.acquireForGroup(*Device, I.Compiled, G.AcquireAtStart, I.Pool);
        rhi::IRhiCommandPool* Cp = I.CommandPools.acquire(*Device, G.QueueKind);
        if (Cp == nullptr)
        {
            GOLETA_LOG_ERROR(RenderGraph,
                             "Failed to acquire command pool for queue kind {}",
                             static_cast<int>(G.QueueKind));
            return;
        }
        Rc<rhi::IRhiCommandList> Cl = Cp->allocate();
        if (!Cl)
        {
            GOLETA_LOG_ERROR(RenderGraph, "Failed to allocate command list");
            return;
        }
        static constexpr const char* QueueShortName[] = {"gfx", "compute", "copy", "video"};
        const int                    Qi = static_cast<int>(G.QueueKind);
        const char*                  Qn = Qi >= 0 && Qi < 4 ? QueueShortName[Qi] : "q";
        const std::string CmdName = "rg." + I.DebugName + "." + Qn + "." + std::to_string(Gi);
        Cl->setDebugName(CmdName.c_str());

        const bool IsLast = (Gi + 1 == I.Schedule.Groups.size());
        std::vector<rhi::RhiTextureBarrier> TexScratch;
        std::vector<rhi::RhiBufferBarrier>  BufScratch;
        // RenderContext's cmd list is swapped per recording; a per-thread local copy keeps
        // the global Ctx from being clobbered by concurrent workers. When only one thread
        // records, the cost is a single pointer swap per group.
        RenderContext LocalCtx = Ctx;
        recordGroup(I, LocalCtx, Cl.get(), G, IsLast, TexScratch, BufScratch);
        Recorded[Gi].CmdList = std::move(Cl);
        Recorded[Gi].Queue   = G.QueueKind;
    };

#if GOLETA_RG_HAS_ENKITS
    if (I.TaskScheduler != nullptr && I.Schedule.Groups.size() > 1)
    {
        struct GroupTask : enki::ITaskSet
        {
            decltype(recordOne)* Fn = nullptr;
            void ExecuteRange(enki::TaskSetPartition R, uint32_t) override
            {
                for (uint32_t i = R.start; i < R.end; ++i)
                    (*Fn)(i);
            }
        };
        GroupTask Task;
        Task.m_SetSize = static_cast<uint32_t>(I.Schedule.Groups.size());
        Task.m_MinRange = 1;
        Task.Fn         = &recordOne;
        I.TaskScheduler->AddTaskSetToPipe(&Task);
        I.TaskScheduler->WaitforTask(&Task);
    }
    else
#endif
    {
        for (uint32_t Gi = 0; Gi < I.Schedule.Groups.size(); ++Gi)
            recordOne(Gi);
    }

    Rc<rhi::IRhiDebug> Debug;
    bool               CaptureActive = false;
    if (I.CaptureRequested)
    {
        Debug = Device->debug();
        if (Debug)
        {
            const std::string& N = I.CaptureName.empty() ? I.DebugName : I.CaptureName;
            Debug->beginCapture(N.c_str());
            CaptureActive = true;
        }
        I.CaptureRequested = false;
        I.CaptureName.clear();
    }

    // Submit groups in schedule order on the main thread.
    for (uint32_t Gi = 0; Gi < I.Schedule.Groups.size(); ++Gi)
    {
        const auto&              G  = I.Schedule.Groups[Gi];
        rhi::IRhiCommandList*    Cl = Recorded[Gi].CmdList.get();
        if (Cl == nullptr)
            continue;

        Rc<rhi::IRhiQueue> Q = Device->getQueue(G.QueueKind);
        if (!Q)
        {
            GOLETA_LOG_ERROR(RenderGraph,
                             "Device missing queue kind {}; graph submission aborted.",
                             static_cast<int>(G.QueueKind));
            break;
        }

        std::vector<rhi::RhiFenceWait>   Waits;
        Waits.reserve(G.WaitGroups.size());
        for (uint32_t WG : G.WaitGroups)
        {
            rhi::RhiFenceWait W;
            W.Fence = I.TimelineFence.get();
            W.Value = FrameBase + I.Schedule.Groups[WG].SignalOffset;
            Waits.push_back(W);
        }

        rhi::RhiFenceSignal Signal;
        Signal.Fence = I.TimelineFence.get();
        Signal.Value = FrameBase + G.SignalOffset;

        // Release transients whose lifetime ends with this group; the pool can now recycle
        // them for subsequent groups in the same frame.
        I.Registry.releaseAfterGroup(I.Compiled, G.ReleaseAfter, I.Pool);

        rhi::IRhiCommandList* const CmdArray[1] = {Cl};
        rhi::RhiSubmitInfo Submit;
        Submit.CommandLists     = CmdArray;
        Submit.CommandListCount = 1;
        Submit.WaitFences       = Waits.empty() ? nullptr : Waits.data();
        Submit.WaitFenceCount   = static_cast<uint32_t>(Waits.size());
        Submit.SignalFences     = &Signal;
        Submit.SignalFenceCount = 1;

        auto R = Q->submit(Submit);
        if (R.isErr())
        {
            GOLETA_LOG_ERROR(RenderGraph, "Queue submit failed for group {}", Gi);
        }
    }

    if (CaptureActive && Debug)
        Debug->endCapture();

    I.FenceFrameBase = FrameBase + I.Schedule.FenceValuesPerFrame;

    if (I.TimingsEnabled && I.TimestampCapacity > 0)
    {
        // CPU timings are ready immediately; GPU ticks fill in on the next execute after the
        // resolve finishes. LastFrame is the *committed* view shown to the user.
        I.TimingsLastFrame         = I.TimingsInFlight;
        I.PendingResolveFenceValue = I.FenceFrameBase;
    }

    I.Registry.endFrame(I.Compiled, I.Pool);
}

} // namespace goleta::rg
