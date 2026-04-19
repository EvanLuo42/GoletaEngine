#include "AccessStateTable.h"

namespace goleta::rg::detail
{

RgBindAs inferTextureBind(RgBindAs Declared, rhi::RhiTextureUsage Usage) noexcept
{
    if (Declared != RgBindAs::Auto)
        return Declared;

    using U = rhi::RhiTextureUsage;
    if (anyOf(Usage, U::DepthAttachment))
        return RgBindAs::DepthAttachment;
    if (anyOf(Usage, U::ColorAttachment))
        return RgBindAs::ColorAttachment;
    if (anyOf(Usage, U::Storage))
        return RgBindAs::Storage;
    if (anyOf(Usage, U::Sampled))
        return RgBindAs::Sampled;
    if (anyOf(Usage, U::CopyDest))
        return RgBindAs::CopyDst;
    if (anyOf(Usage, U::CopySource))
        return RgBindAs::CopySrc;
    return RgBindAs::Sampled;
}

TextureAccessState textureStateFor(RgBindAs Bind, RgAccessMode Mode) noexcept
{
    using L = rhi::RhiTextureLayout;
    using A = rhi::RhiAccess;
    using P = rhi::RhiPipelineStage;

    TextureAccessState S;
    const bool Writes = Mode == RgAccessMode::Output || Mode == RgAccessMode::InputOutput;
    const bool Reads  = Mode == RgAccessMode::Input || Mode == RgAccessMode::InputOutput;

    switch (Bind)
    {
    case RgBindAs::Sampled:
        S.Layout = L::ShaderResource;
        S.Access = A::ShaderResourceRead;
        S.Stages = P::PixelShader | P::ComputeShader | P::VertexShader;
        S.Writes = false;
        break;
    case RgBindAs::Storage:
        S.Layout = L::UnorderedAccess;
        S.Access = (Reads ? A::UnorderedAccessRead : A::None) |
                   (Writes ? A::UnorderedAccessWrite : A::None);
        S.Stages = P::ComputeShader | P::PixelShader;
        S.Writes = Writes;
        break;
    case RgBindAs::ColorAttachment:
        S.Layout = L::ColorAttachment;
        S.Access = (Reads ? A::ColorAttachmentRead : A::None) |
                   (Writes ? A::ColorAttachmentWrite : A::None);
        S.Stages = P::ColorAttachmentOut;
        S.Writes = Writes;
        break;
    case RgBindAs::DepthAttachment:
        S.Layout = Writes ? L::DepthStencilWrite : L::DepthStencilRead;
        S.Access = (Reads ? A::DepthRead : A::None) | (Writes ? A::DepthWrite : A::None);
        S.Stages = P::EarlyFragmentTests | P::LateFragmentTests;
        S.Writes = Writes;
        break;
    case RgBindAs::CopySrc:
        S.Layout = L::CopySource;
        S.Access = A::CopyRead;
        S.Stages = P::Copy;
        S.Writes = false;
        break;
    case RgBindAs::CopyDst:
        S.Layout = L::CopyDest;
        S.Access = A::CopyWrite;
        S.Stages = P::Copy;
        S.Writes = true;
        break;
    case RgBindAs::Auto:
    default:
        S.Layout = L::ShaderResource;
        S.Access = A::ShaderResourceRead;
        S.Stages = P::PixelShader | P::ComputeShader;
        S.Writes = false;
        break;
    }
    return S;
}

BufferAccessState bufferStateFor(RgBindAs Bind, RgAccessMode Mode,
                                 rhi::RhiBufferUsage Usage) noexcept
{
    using A = rhi::RhiAccess;
    using P = rhi::RhiPipelineStage;
    using BU = rhi::RhiBufferUsage;

    BufferAccessState S;
    const bool Writes = Mode == RgAccessMode::Output || Mode == RgAccessMode::InputOutput;
    const bool Reads  = Mode == RgAccessMode::Input || Mode == RgAccessMode::InputOutput;

    // Prefer an explicit BindAs; fall back to usage bits.
    if (Bind == RgBindAs::Storage ||
        (Bind == RgBindAs::Auto && anyOf(Usage, BU::StorageBuffer)))
    {
        S.Access = (Reads ? A::UnorderedAccessRead : A::None) |
                   (Writes ? A::UnorderedAccessWrite : A::None);
        S.Stages = P::ComputeShader | P::PixelShader;
        S.Writes = Writes;
    }
    else if (Bind == RgBindAs::CopyDst || anyOf(Usage, BU::CopyDest))
    {
        S.Access = A::CopyWrite;
        S.Stages = P::Copy;
        S.Writes = true;
    }
    else if (Bind == RgBindAs::CopySrc || anyOf(Usage, BU::CopySource))
    {
        S.Access = A::CopyRead;
        S.Stages = P::Copy;
        S.Writes = false;
    }
    else if (anyOf(Usage, BU::IndirectBuffer))
    {
        S.Access = A::IndirectRead;
        S.Stages = P::DrawIndirect;
        S.Writes = false;
    }
    else if (anyOf(Usage, BU::ConstantBuffer))
    {
        S.Access = A::ConstantBufferRead;
        S.Stages = P::VertexShader | P::PixelShader | P::ComputeShader;
        S.Writes = false;
    }
    else
    {
        // Default: structured/raw read.
        S.Access = A::ShaderResourceRead;
        S.Stages = P::VertexShader | P::PixelShader | P::ComputeShader;
        S.Writes = false;
    }
    return S;
}

} // namespace goleta::rg::detail
