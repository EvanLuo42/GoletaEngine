#pragma once

/// @file
/// @brief Per-pass, read-only view of the graph's physical resources, addressed by the typed
///        PassField handles the pass declared in reflect().

#include "PassField.h"
#include "RHIHandle.h"
#include "RenderGraphExport.h"

namespace goleta::rhi
{
class IRhiTexture;
class IRhiBuffer;
} // namespace goleta::rhi

namespace goleta::rg
{

struct CompiledGraph;
class  ResourceRegistry;

/// @brief Handed to each pass's execute(). Resolves typed handles to the physical resources
///        the compiled graph allocated for the current frame.
class RENDERGRAPH_API RenderData
{
public:
    /// @brief Resolve a texture input to its physical RHI texture. Asserts the handle belongs
    ///        to the currently executing pass.
    [[nodiscard]] rhi::IRhiTexture* getTexture(PassInput<Texture> H) const noexcept;
    [[nodiscard]] rhi::IRhiTexture* getTexture(PassOutput<Texture> H) const noexcept;
    [[nodiscard]] rhi::IRhiTexture* getTexture(PassInputOutput<Texture> H) const noexcept;

    /// @brief Resolve a buffer input to its physical RHI buffer.
    [[nodiscard]] rhi::IRhiBuffer* getBuffer(PassInput<Buffer> H) const noexcept;
    [[nodiscard]] rhi::IRhiBuffer* getBuffer(PassOutput<Buffer> H) const noexcept;
    [[nodiscard]] rhi::IRhiBuffer* getBuffer(PassInputOutput<Buffer> H) const noexcept;

    /// @brief Bindless SRV index for indirect / shader-visible paths.
    [[nodiscard]] rhi::RhiTextureHandle getTextureSrv(PassInput<Texture> H) const noexcept;

    /// @brief Bindless UAV index for indirect / shader-visible paths.
    [[nodiscard]] rhi::RhiRwTextureHandle getTextureUav(PassOutput<Texture> H) const noexcept;
    [[nodiscard]] rhi::RhiRwTextureHandle getTextureUav(PassInputOutput<Texture> H) const noexcept;

    /// @brief Internal: initialise the lookup state. Only RenderGraph and its private impl
    ///        helpers are expected to call this; the separation keeps pass-authoring code
    ///        from ever touching it.
    void bind(const CompiledGraph* Compiled, const ResourceRegistry* Registry,
              PassId CurrentPass) noexcept
    {
        Compiled_    = Compiled;
        Registry_    = Registry;
        CurrentPass_ = CurrentPass;
    }

private:
    rhi::IRhiTexture* resolveTexture(PassId Pass, FieldId Field) const noexcept;
    rhi::IRhiBuffer*  resolveBuffer(PassId Pass, FieldId Field) const noexcept;

    const CompiledGraph*    Compiled_    = nullptr;
    const ResourceRegistry* Registry_    = nullptr;
    PassId                  CurrentPass_ = PassId::Invalid;
};

} // namespace goleta::rg
