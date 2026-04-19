#pragma once

/// @file
/// @brief Precompiled shader-blob wrapper. RHI does not compile source — a separate
///        ShaderCompiler module produces DXIL / SPIR-V / platform-private blobs.

#include <cstddef>
#include <cstdint>

#include "RHIEnums.h"
#include "RHIExport.h"
#include "RHIResource.h"
#include "RHIStructChain.h"

namespace goleta::rhi
{

/// @brief Creation parameters for a shader module.
/// @note  The blob is copied by the backend at creation time; the caller may free Bytecode after createShaderModule()
/// returns.
struct RhiShaderModuleDesc
{
    static constexpr auto kStructType = RhiStructType::ShaderModuleDesc;
    RhiStructHeader       Header{kStructType, nullptr};

    RhiShaderKind  Kind         = RhiShaderKind::DXIL;
    RhiShaderStage Stage        = RhiShaderStage::Vertex;
    const void*    Bytecode     = nullptr;
    size_t         BytecodeSize = 0;
    const char*    EntryPoint   = "main";

    const char* DebugName = nullptr;
};

/// @brief Compiled shader blob wrapped into a device-side module.
class RHI_API IRhiShaderModule : public IRhiResource
{
public:
    virtual const RhiShaderModuleDesc& desc() const noexcept = 0;
    /// @brief Bytecode family (DXIL / SPIR-V / platform-private). IRhiResource::kind() still
    ///        returns RhiResourceKind::ShaderModule for runtime type checks.
    virtual RhiShaderKind  shaderKind() const noexcept = 0;
    virtual RhiShaderStage stage() const noexcept      = 0;
};

} // namespace goleta::rhi
