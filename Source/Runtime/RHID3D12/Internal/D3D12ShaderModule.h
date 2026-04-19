#pragma once

/// @file
/// @brief Lightweight IRhiShaderModule that takes ownership of a DXIL/DXBC blob.

#include <cstddef>
#include <string>
#include <vector>

#include "D3D12Prelude.h"
#include "RHIShader.h"

namespace goleta::rhi::d3d12
{

class D3D12ShaderModule final : public IRhiShaderModule
{
public:
    static Rc<D3D12ShaderModule> create(const RhiShaderModuleDesc& Desc) noexcept;

    static constexpr RhiResourceKind kExpectedKind = RhiResourceKind::ShaderModule;

    // IRhiResource
    RhiResourceKind kind() const noexcept override { return kExpectedKind; }
    const char*     debugName() const noexcept override { return Name.c_str(); }
    void            setDebugName(const char* NewName) override { Name = NewName ? NewName : ""; }

    // IRhiShaderModule
    const RhiShaderModuleDesc& desc() const noexcept override { return Desc_; }
    RhiShaderKind              shaderKind() const noexcept override { return Desc_.Kind; }
    RhiShaderStage             stage() const noexcept override { return Desc_.Stage; }

    D3D12_SHADER_BYTECODE bytecode() const noexcept
    {
        return D3D12_SHADER_BYTECODE{Blob.data(), Blob.size()};
    }

private:
    D3D12ShaderModule() noexcept = default;

    RhiShaderModuleDesc    Desc_{};
    std::vector<std::byte> Blob;
    std::string            EntryPoint;
    std::string            Name;
};

} // namespace goleta::rhi::d3d12
