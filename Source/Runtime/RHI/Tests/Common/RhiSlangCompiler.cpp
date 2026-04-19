/// @file
/// @brief Slang → backend bytecode. One global session cached per process; a fresh compile
///        session per call selects the target format.

#include "RhiSlangCompiler.h"

#ifdef GOLETA_RHID3D12_HAS_SLANG
#  include <slang-com-ptr.h>
#  include <slang.h>
#endif

#include <cstdlib>
#include <filesystem>
#include <mutex>

namespace goleta::rhi::tests
{

#ifdef GOLETA_RHID3D12_HAS_SLANG
namespace
{

std::once_flag                       g_SessionOnce;
Slang::ComPtr<slang::IGlobalSession> g_Global;

slang::IGlobalSession* ensureGlobal() noexcept
{
    std::call_once(g_SessionOnce, []() {
        slang::IGlobalSession* Raw = nullptr;
        slang::createGlobalSession(&Raw);
        g_Global.attach(Raw);
    });
    return g_Global.get();
}

SlangCompileTarget selectTarget(BackendKind Kind) noexcept
{
    switch (Kind)
    {
    case BackendKind::D3D12:     return SLANG_DXIL;
    case BackendKind::D3D12Xbox: return SLANG_DXIL;
    case BackendKind::Vulkan:    return SLANG_SPIRV;
    default:                     return SLANG_TARGET_UNKNOWN;
    }
}

} // namespace
#endif

bool slangRuntimeAvailable() noexcept
{
#ifdef GOLETA_RHID3D12_HAS_SLANG
    return ensureGlobal() != nullptr;
#else
    return false;
#endif
}

RhiShaderKind shaderKindFor(BackendKind Kind) noexcept
{
    switch (Kind)
    {
    case BackendKind::D3D12:     return RhiShaderKind::DXIL;
    case BackendKind::D3D12Xbox: return RhiShaderKind::DXIL;
    case BackendKind::Vulkan:    return RhiShaderKind::SPIRV;
    case BackendKind::GNMX:      return RhiShaderKind::PSSL;
    case BackendKind::NVN:       return RhiShaderKind::NVNShader;
    default:                     return RhiShaderKind::DXIL;
    }
}

std::string slangShaderPath(const std::string& Filename)
{
    if (const char* Dir = std::getenv("GOLETA_TEST_SHADER_DIR"))
    {
        std::filesystem::path P(Dir);
        P /= Filename;
        return P.string();
    }
    std::filesystem::path P = std::filesystem::current_path() / "TestShaders" / Filename;
    return P.string();
}

#ifdef GOLETA_RHID3D12_HAS_SLANG
SlangCompileResult compileSlangForBackend(BackendKind Backend, const std::string& ModulePath,
                                          const std::string& EntryPoint)
{
    SlangCompileResult Out;

    auto* Global = ensureGlobal();
    if (!Global)
    {
        Out.Diagnostics = "slang not available";
        return Out;
    }

    const SlangCompileTarget Target = selectTarget(Backend);
    if (Target == SLANG_TARGET_UNKNOWN)
    {
        Out.Diagnostics = "no slang target for this backend";
        return Out;
    }

    slang::SessionDesc SessionDesc{};
    slang::TargetDesc  TargetDesc{};
    TargetDesc.format  = Target;
    TargetDesc.profile = Global->findProfile(Target == SLANG_DXIL ? "sm_6_6" : "spirv_1_5");
    TargetDesc.flags   = 0;

    SessionDesc.targets                 = &TargetDesc;
    SessionDesc.targetCount             = 1;
    SessionDesc.defaultMatrixLayoutMode = SLANG_MATRIX_LAYOUT_COLUMN_MAJOR;

    Slang::ComPtr<slang::ISession> Session;
    if (SLANG_FAILED(Global->createSession(SessionDesc, Session.writeRef())))
    {
        Out.Diagnostics = "createSession failed";
        return Out;
    }

    Slang::ComPtr<slang::IBlob>   DiagBlob;
    Slang::ComPtr<slang::IModule> Module(Session->loadModule(ModulePath.c_str(), DiagBlob.writeRef()));
    if (!Module)
    {
        if (DiagBlob) Out.Diagnostics = static_cast<const char*>(DiagBlob->getBufferPointer());
        return Out;
    }

    Slang::ComPtr<slang::IEntryPoint> Ep;
    if (SLANG_FAILED(Module->findEntryPointByName(EntryPoint.c_str(), Ep.writeRef())))
    {
        Out.Diagnostics = "entry point not found: " + EntryPoint;
        return Out;
    }

    slang::IComponentType*               Parts[] = {Module.get(), Ep.get()};
    Slang::ComPtr<slang::IComponentType> Composed;
    if (SLANG_FAILED(Session->createCompositeComponentType(Parts, 2, Composed.writeRef(), DiagBlob.writeRef())))
    {
        if (DiagBlob) Out.Diagnostics = static_cast<const char*>(DiagBlob->getBufferPointer());
        return Out;
    }

    Slang::ComPtr<slang::IComponentType> Linked;
    if (SLANG_FAILED(Composed->link(Linked.writeRef(), DiagBlob.writeRef())))
    {
        if (DiagBlob) Out.Diagnostics = static_cast<const char*>(DiagBlob->getBufferPointer());
        return Out;
    }

    Slang::ComPtr<slang::IBlob> Code;
    if (SLANG_FAILED(Linked->getEntryPointCode(0, 0, Code.writeRef(), DiagBlob.writeRef())))
    {
        if (DiagBlob) Out.Diagnostics = static_cast<const char*>(DiagBlob->getBufferPointer());
        return Out;
    }
    const auto*  Ptr = static_cast<const std::byte*>(Code->getBufferPointer());
    const size_t N   = Code->getBufferSize();
    Out.Bytecode.assign(Ptr, Ptr + N);
    return Out;
}
#else
SlangCompileResult compileSlangForBackend(BackendKind, const std::string&, const std::string&)
{
    SlangCompileResult Out;
    Out.Diagnostics = "slang prebuilt not fetched at configure time";
    return Out;
}
#endif

} // namespace goleta::rhi::tests
