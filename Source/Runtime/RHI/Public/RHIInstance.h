#pragma once

/// @file
/// @brief Entry-point factory: createInstance() + IRhiInstance.

#include <vector>

#include "Memory/Rc.h"
#include "RHIAdapter.h"
#include "RHIDebug.h"
#include "RHIEnums.h"
#include "RHIExport.h"
#include "RHIStructChain.h"

namespace goleta::rhi
{

class IRhiDevice;
struct RhiDeviceCreateInfo;

/// @brief Creation parameters for an RHI instance.
struct RhiInstanceCreateInfo
{
    static constexpr RhiStructType kStructType = RhiStructType::InstanceCreateInfo;
    RhiStructHeader                Header{kStructType, nullptr};

    BackendKind      Backend             = BackendKind::Null;
    RhiDebugLevel    DebugLevel          = RhiDebugLevel::None;
    RhiDebugCallback MessageCallback     = nullptr;
    void*            MessageCallbackUser = nullptr;

    const char* ApplicationName    = "Goleta";
    uint32_t    ApplicationVersion = 0;
    const char* EngineName         = "GoletaEngine";
    uint32_t    EngineVersion      = 0;
};

/// @brief Top-level factory. One instance per backend per process is the common case.
class RHI_API IRhiInstance : public RefCounted
{
public:
    virtual BackendKind backend() const noexcept = 0;

    virtual std::vector<RhiAdapterInfo> enumerateAdapters() const = 0;

    virtual Rc<IRhiDevice> createDevice(const RhiDeviceCreateInfo& Desc) = 0;
};

/// @brief Construct an instance for the backend named in Desc.Backend. Returns a null Rc when
///        no backend of that kind is registered.
RHI_API Rc<IRhiInstance> createInstance(const RhiInstanceCreateInfo& Desc);

} // namespace goleta::rhi
