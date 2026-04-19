#pragma once

/// @file
/// @brief Native-handle trap door. Lets platform-specific code reach past the abstraction to
///        raw D3D12 / Vulkan / console objects without exposing those SDK headers here.

#include "../RHIExport.h"
#include "../RHINativeHandle.h"

namespace goleta::rhi
{

class IRhiDevice;
class IRhiQueue;
class IRhiCommandList;
class IRhiBuffer;
class IRhiTexture;
class IRhiFence;

RHI_API RhiNativeHandle getNativeHandle(const IRhiDevice* Device) noexcept;
RHI_API RhiNativeHandle getNativeHandle(IRhiQueue* Queue) noexcept;
RHI_API RhiNativeHandle getNativeHandle(IRhiCommandList* CommandList) noexcept;
RHI_API RhiNativeHandle getNativeHandle(IRhiBuffer* Buffer) noexcept;
RHI_API RhiNativeHandle getNativeHandle(IRhiTexture* Texture) noexcept;
RHI_API RhiNativeHandle getNativeHandle(IRhiFence* Fence) noexcept;

} // namespace goleta::rhi
