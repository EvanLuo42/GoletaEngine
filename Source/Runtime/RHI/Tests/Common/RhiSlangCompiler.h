#pragma once

/// @file
/// @brief Slang → (DXIL | SPIR-V | ...) wrapper. Selects the target format based on the
///        BackendKind under test. Returns an empty blob + diagnostic string on failure.

#include <cstddef>
#include <string>
#include <vector>

#include "RHIEnums.h"

namespace goleta::rhi::tests
{

struct SlangCompileResult
{
    std::vector<std::byte> Bytecode;
    std::string            Diagnostics;

    bool isOk() const noexcept { return !Bytecode.empty(); }
};

/// @brief Compile one entry point from a .slang module to the native bytecode of `TargetBackend`.
SlangCompileResult compileSlangForBackend(BackendKind TargetBackend, const std::string& ModulePath,
                                          const std::string& EntryPoint);

/// @brief Absolute path to a .slang file under the TestShaders/ directory next to the executable.
std::string slangShaderPath(const std::string& Filename);

/// @brief Whether the slang runtime library is loaded and ready.
bool slangRuntimeAvailable() noexcept;

/// @brief The RhiShaderKind matching a BackendKind (DXIL for D3D12, SPIRV for Vulkan, etc.).
RhiShaderKind shaderKindFor(BackendKind Kind) noexcept;

} // namespace goleta::rhi::tests
