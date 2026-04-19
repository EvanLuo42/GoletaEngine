#pragma once

/// @file
/// @brief Per-frame context threaded into each pass's execute(): device, active command list,
///        frame index, and a typeHash-keyed user-data blackboard for scene-side data.

#include <cstdint>
#include <unordered_map>

#include "RenderGraphExport.h"
#include "TypeHash.h"

namespace goleta::rhi
{
class IRhiDevice;
class IRhiCommandList;
} // namespace goleta::rhi

namespace goleta::rg
{

class RenderGraph;

/// @brief Caller-owned execution context. The graph mutates ActiveCmdList_ per pass group.
class RENDERGRAPH_API RenderContext
{
public:
    RenderContext() = default;
    RenderContext(rhi::IRhiDevice* Dev, rhi::IRhiCommandList* Cmd, uint64_t FrameIdx = 0) noexcept
        : Device_(Dev), ActiveCmdList_(Cmd), FrameIndex_(FrameIdx)
    {
    }

    [[nodiscard]] rhi::IRhiDevice*      device() noexcept { return Device_; }
    [[nodiscard]] rhi::IRhiCommandList* cmdList() noexcept { return ActiveCmdList_; }
    [[nodiscard]] uint64_t              frameIndex() const noexcept { return FrameIndex_; }

    void setDevice(rhi::IRhiDevice* Dev) noexcept { Device_ = Dev; }
    void setCmdList(rhi::IRhiCommandList* Cmd) noexcept { ActiveCmdList_ = Cmd; }
    void setFrameIndex(uint64_t Idx) noexcept { FrameIndex_ = Idx; }

    /// @brief Fetch a user-data pointer previously set via setUserData<T>(). Nullptr if unset.
    template <class T>
    [[nodiscard]] T* getUserData() noexcept
    {
        auto It = UserData_.find(typeHash<T>());
        return It == UserData_.end() ? nullptr : static_cast<T*>(It->second);
    }

    /// @brief Publish a user-data pointer (not owned). Scene and lighting subsystems use this
    ///        to ferry their per-frame structures into passes.
    template <class T>
    void setUserData(T* Ptr) noexcept
    {
        UserData_[typeHash<T>()] = static_cast<void*>(Ptr);
    }

private:
    friend class RenderGraph;
    rhi::IRhiDevice*      Device_        = nullptr;
    rhi::IRhiCommandList* ActiveCmdList_ = nullptr;
    uint64_t              FrameIndex_    = 0;

    std::unordered_map<uint64_t, void*> UserData_;
};

} // namespace goleta::rg
