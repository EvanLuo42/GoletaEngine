/// @file
/// @brief Free-function forwarders that dispatch to IRhiResource::nativeHandle() /
///        IRhiDevice::nativeHandle().

#include "Platform/RHIPlatform.h"

#include "RHIBuffer.h"
#include "RHICommandList.h"
#include "RHIDevice.h"
#include "RHIQueue.h"
#include "RHISync.h"
#include "RHITexture.h"

namespace goleta::rhi
{

RhiNativeHandle getNativeHandle(const IRhiDevice* Device) noexcept
{
    return Device ? Device->nativeHandle() : RhiNativeHandle{};
}

RhiNativeHandle getNativeHandle(const IRhiQueue* Queue) noexcept
{
    return Queue ? Queue->nativeHandle() : RhiNativeHandle{};
}

RhiNativeHandle getNativeHandle(const IRhiCommandList* CommandList) noexcept
{
    return CommandList ? CommandList->nativeHandle() : RhiNativeHandle{};
}

RhiNativeHandle getNativeHandle(const IRhiBuffer* Buffer) noexcept
{
    return Buffer ? Buffer->nativeHandle() : RhiNativeHandle{};
}

RhiNativeHandle getNativeHandle(const IRhiTexture* Texture) noexcept
{
    return Texture ? Texture->nativeHandle() : RhiNativeHandle{};
}

RhiNativeHandle getNativeHandle(const IRhiFence* Fence) noexcept
{
    return Fence ? Fence->nativeHandle() : RhiNativeHandle{};
}

} // namespace goleta::rhi
