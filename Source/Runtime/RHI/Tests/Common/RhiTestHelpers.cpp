/// @file
/// @brief Cross-backend test helper implementations.

#include "RhiTestHelpers.h"

namespace goleta::rhi::tests
{

void submitAndWait(IRhiDevice* Device, IRhiQueue* Queue, IRhiCommandList* CmdList)
{
    auto Fence = Device->createFence(0);
    ASSERT_TRUE(Fence);
    IRhiCommandList* Lists[1] = {CmdList};
    RhiFenceSignal   Sig{Fence.get(), 1};
    RhiSubmitInfo    Info{};
    Info.CommandLists     = Lists;
    Info.CommandListCount = 1;
    Info.SignalFences     = &Sig;
    Info.SignalFenceCount = 1;
    REQUIRE_RHI_OK(Queue->submit(Info));
    const auto WaitR = Fence->wait(1);
    ASSERT_TRUE(WaitR.isOk());
}

} // namespace goleta::rhi::tests
