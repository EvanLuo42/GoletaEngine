/// @file
/// @brief Cross-backend RhiTest fixture implementation.

#include "RhiTestFixture.h"

#include <cstdio>
#include <cstring>
#include <mutex>

#include "RHIBackend.h"

#ifdef GOLETA_RHID3D12_HAS_FORCELINK
#  include "RHID3D12ForceLink.h"
#endif

namespace goleta::rhi::tests
{
namespace
{

std::once_flag   g_ForceLinkFlag;
BackendState     g_BackendStates[static_cast<size_t>(BackendKind::NVN) + 1];
std::mutex       g_BackendMutex;

void forceLinkAllBackends() noexcept
{
#ifdef GOLETA_RHID3D12_HAS_FORCELINK
    forceLinkRhiD3D12Backend();
#endif
    // TODO(rhi): forceLinkRhiVulkanBackend() / others as they land.
}

void probeBackend(BackendState& S, BackendKind Kind) noexcept
{
    if (findRhiBackend(Kind) == nullptr)
    {
        S.Available = false;
        return;
    }
    RhiInstanceCreateInfo II{};
    II.Backend    = Kind;
    II.DebugLevel = RhiDebugLevel::Validation;
    II.MessageCallback = [](const RhiDebugMessage& Msg, void*) {
        if (Msg.Severity >= RhiDebugSeverity::Warning)
            std::fprintf(stderr, "[%s-Debug] %s\n", backendName(BackendKind::D3D12),
                         Msg.Message ? Msg.Message : "");
    };
    S.Instance = createInstance(II);
    if (!S.Instance)
    {
        S.Available = false;
        return;
    }
    RhiDeviceCreateInfo DI{};
    S.Device = S.Instance->createDevice(DI);
    if (!S.Device)
    {
        S.Available = false;
        S.Instance.reset();
        return;
    }
    S.Gfx = S.Device->getQueue(RhiQueueKind::Graphics);
    S.Available = (S.Gfx != nullptr);
}

} // namespace

const char* backendName(BackendKind Kind) noexcept
{
    switch (Kind)
    {
    case BackendKind::Null:      return "Null";
    case BackendKind::D3D12:     return "D3D12";
    case BackendKind::Vulkan:    return "Vulkan";
    case BackendKind::D3D12Xbox: return "D3D12Xbox";
    case BackendKind::GNMX:      return "GNMX";
    case BackendKind::NVN:       return "NVN";
    }
    return "Unknown";
}

BackendState& backendState(BackendKind Kind) noexcept
{
    std::call_once(g_ForceLinkFlag, []() { forceLinkAllBackends(); });
    auto& S = g_BackendStates[static_cast<size_t>(Kind)];
    std::scoped_lock Lock(g_BackendMutex);
    if (!S.Probed)
    {
        probeBackend(S, Kind);
        S.Probed = true;
    }
    return S;
}

void RhiTest::SetUp()
{
    const BackendKind K = GetParam();
    auto&             S = backendState(K);
    if (!S.Available)
    {
        GTEST_SKIP() << "backend " << backendName(K) << " not available";
    }
    Device = S.Device.get();
    Gfx    = S.Gfx.get();
}

// The list of backends to attempt per test. INSTANTIATE_TEST_SUITE_P evaluates Values() at
// static-init time (before main), so we hardcode the complete BackendKind roster; probing
// happens lazily per-backend on first SetUp.
INSTANTIATE_TEST_SUITE_P(
    Backend,
    RhiTest,
    ::testing::Values(BackendKind::Null, BackendKind::D3D12),
    [](const ::testing::TestParamInfo<BackendKind>& Info) {
        return std::string(backendName(Info.param));
    });

} // namespace goleta::rhi::tests
