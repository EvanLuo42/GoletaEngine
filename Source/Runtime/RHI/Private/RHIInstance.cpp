/// @file
/// @brief createInstance() entry point. Dispatches through the registry; returns a null Rc
///        when no factory is registered for the requested backend.

#include "RHIInstance.h"

#include "RHIBackend.h"

namespace goleta::rhi
{

Rc<IRhiInstance> createInstance(const RhiInstanceCreateInfo& Desc)
{
    const RhiBackendFactoryFn Factory = findRhiBackend(Desc.Backend);
    if (!Factory)
        return Rc<IRhiInstance>{};
    return Factory(Desc);
}

} // namespace goleta::rhi
