#pragma once

/// @file
/// @brief Base interface for everything the device owns. All resources are Rc<T>-managed.

#include "Memory/Rc.h"
#include "RHIEnums.h"
#include "RHIExport.h"
#include "RHINativeHandle.h"

namespace goleta::rhi
{

/// @brief Root interface for every GPU-visible resource handed out by the RHI.
class RHI_API IRhiResource : public RefCounted
{
public:
    /// @brief Runtime discriminator for narrowing down an IRhiResource* to a concrete type.
    [[nodiscard]] virtual RhiResourceKind kind() const noexcept = 0;

    /// @brief Debug name currently attached to this resource. Empty string if unset.
    /// @note  Pointer is valid until the next setDebugName() call or resource destruction.
    [[nodiscard]] virtual const char* debugName() const noexcept = 0;

    /// @brief Attach a debug name. Forwarded to the backend's debug facility
    ///        (ID3D12Object::SetName / vkSetDebugUtilsObjectNameEXT / platform equivalent).
    /// @param Name UTF-8, null-terminated. Backends copy; the caller may free immediately.
    virtual void setDebugName(const char* Name) = 0;

    /// @brief Native handle trap door. Returns {Unknown, nullptr} by default; a backend that
    ///        wraps a native object (D3D12 resource, Vulkan handle, console equivalent) overrides
    ///        this to surface it. Used by Platform/RHIPlatform.h::getNativeHandle().
    [[nodiscard]] virtual RhiNativeHandle nativeHandle() const noexcept { return RhiNativeHandle{}; }

protected:
    IRhiResource() = default;
};

} // namespace goleta::rhi
