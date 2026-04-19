/// @file
/// @brief DXIL blob holder.

#include "D3D12ShaderModule.h"

namespace goleta::rhi::d3d12
{

Rc<D3D12ShaderModule> D3D12ShaderModule::create(const RhiShaderModuleDesc& Desc) noexcept
{
    if (!Desc.Bytecode || Desc.BytecodeSize == 0)
        return {};
    auto Self = Rc<D3D12ShaderModule>(new D3D12ShaderModule{});
    Self->Desc_ = Desc;
    Self->Blob.assign(static_cast<const std::byte*>(Desc.Bytecode),
                      static_cast<const std::byte*>(Desc.Bytecode) + Desc.BytecodeSize);
    Self->Desc_.Bytecode = Self->Blob.data();
    Self->EntryPoint     = Desc.EntryPoint ? Desc.EntryPoint : "main";
    Self->Desc_.EntryPoint = Self->EntryPoint.c_str();
    if (Desc.DebugName)
        Self->Name = Desc.DebugName;
    return Self;
}

} // namespace goleta::rhi::d3d12
