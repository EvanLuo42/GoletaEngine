/// @file
/// @brief RHI barrier flag → D3D12_BARRIER_* translation.

#include "D3D12BarrierTranslate.h"

namespace goleta::rhi::d3d12
{

D3D12_BARRIER_SYNC toD3dSync(RhiPipelineStage Stages) noexcept
{
    using S = RhiPipelineStage;
    const auto Raw = static_cast<uint32_t>(Stages);
    if (Raw == 0)
        return D3D12_BARRIER_SYNC_NONE;
    if ((Stages & S::AllCommands) == S::AllCommands)
        return D3D12_BARRIER_SYNC_ALL;
    if ((Stages & S::AllGraphics) == S::AllGraphics)
        return D3D12_BARRIER_SYNC_DRAW;

    D3D12_BARRIER_SYNC Out = D3D12_BARRIER_SYNC_NONE;
    if ((Stages & S::DrawIndirect)       != S::None) Out |= D3D12_BARRIER_SYNC_EXECUTE_INDIRECT;
    if ((Stages & S::VertexInput)        != S::None) Out |= D3D12_BARRIER_SYNC_INDEX_INPUT | D3D12_BARRIER_SYNC_VERTEX_SHADING;
    if ((Stages & S::VertexShader)       != S::None) Out |= D3D12_BARRIER_SYNC_VERTEX_SHADING;
    if ((Stages & S::PixelShader)        != S::None) Out |= D3D12_BARRIER_SYNC_PIXEL_SHADING;
    if ((Stages & S::EarlyFragmentTests) != S::None) Out |= D3D12_BARRIER_SYNC_DEPTH_STENCIL;
    if ((Stages & S::LateFragmentTests)  != S::None) Out |= D3D12_BARRIER_SYNC_DEPTH_STENCIL;
    if ((Stages & S::ColorAttachmentOut) != S::None) Out |= D3D12_BARRIER_SYNC_RENDER_TARGET;
    if ((Stages & S::ComputeShader)      != S::None) Out |= D3D12_BARRIER_SYNC_COMPUTE_SHADING;
    if ((Stages & S::RayTracingShader)   != S::None) Out |= D3D12_BARRIER_SYNC_RAYTRACING;
    if ((Stages & S::Copy)               != S::None) Out |= D3D12_BARRIER_SYNC_COPY;
    if ((Stages & S::Resolve)            != S::None) Out |= D3D12_BARRIER_SYNC_RESOLVE;
    if ((Stages & S::AccelerationBuild)  != S::None) Out |= D3D12_BARRIER_SYNC_BUILD_RAYTRACING_ACCELERATION_STRUCTURE;
    return Out;
}

D3D12_BARRIER_ACCESS toD3dAccess(RhiAccess Access) noexcept
{
    using A = RhiAccess;
    if (Access == A::None)
        return D3D12_BARRIER_ACCESS_NO_ACCESS;

    D3D12_BARRIER_ACCESS Out = D3D12_BARRIER_ACCESS_COMMON;
    if ((Access & A::IndirectRead)         != A::None) Out |= D3D12_BARRIER_ACCESS_INDIRECT_ARGUMENT;
    if ((Access & A::IndexRead)            != A::None) Out |= D3D12_BARRIER_ACCESS_INDEX_BUFFER;
    if ((Access & A::VertexRead)           != A::None) Out |= D3D12_BARRIER_ACCESS_VERTEX_BUFFER;
    if ((Access & A::ConstantBufferRead)   != A::None) Out |= D3D12_BARRIER_ACCESS_CONSTANT_BUFFER;
    if ((Access & A::ShaderResourceRead)   != A::None) Out |= D3D12_BARRIER_ACCESS_SHADER_RESOURCE;
    if ((Access & A::UnorderedAccessRead)  != A::None) Out |= D3D12_BARRIER_ACCESS_UNORDERED_ACCESS;
    if ((Access & A::UnorderedAccessWrite) != A::None) Out |= D3D12_BARRIER_ACCESS_UNORDERED_ACCESS;
    if ((Access & A::ColorAttachmentRead)  != A::None) Out |= D3D12_BARRIER_ACCESS_RENDER_TARGET;
    if ((Access & A::ColorAttachmentWrite) != A::None) Out |= D3D12_BARRIER_ACCESS_RENDER_TARGET;
    if ((Access & A::DepthRead)            != A::None) Out |= D3D12_BARRIER_ACCESS_DEPTH_STENCIL_READ;
    if ((Access & A::DepthWrite)           != A::None) Out |= D3D12_BARRIER_ACCESS_DEPTH_STENCIL_WRITE;
    if ((Access & A::CopyRead)             != A::None) Out |= D3D12_BARRIER_ACCESS_COPY_SOURCE;
    if ((Access & A::CopyWrite)            != A::None) Out |= D3D12_BARRIER_ACCESS_COPY_DEST;
    if ((Access & A::AccelRead)            != A::None) Out |= D3D12_BARRIER_ACCESS_RAYTRACING_ACCELERATION_STRUCTURE_READ;
    if ((Access & A::AccelWrite)           != A::None) Out |= D3D12_BARRIER_ACCESS_RAYTRACING_ACCELERATION_STRUCTURE_WRITE;
    if ((Access & A::Present)              != A::None) Out  = D3D12_BARRIER_ACCESS_COMMON; // Present uses COMMON.
    return Out;
}

D3D12_BARRIER_LAYOUT toD3dLayout(RhiTextureLayout Layout) noexcept
{
    switch (Layout)
    {
    case RhiTextureLayout::Undefined:         return D3D12_BARRIER_LAYOUT_UNDEFINED;
    case RhiTextureLayout::Common:            return D3D12_BARRIER_LAYOUT_COMMON;
    case RhiTextureLayout::GenericRead:       return D3D12_BARRIER_LAYOUT_GENERIC_READ;
    case RhiTextureLayout::ShaderResource:    return D3D12_BARRIER_LAYOUT_SHADER_RESOURCE;
    case RhiTextureLayout::UnorderedAccess:   return D3D12_BARRIER_LAYOUT_UNORDERED_ACCESS;
    case RhiTextureLayout::ColorAttachment:   return D3D12_BARRIER_LAYOUT_RENDER_TARGET;
    case RhiTextureLayout::DepthStencilWrite: return D3D12_BARRIER_LAYOUT_DEPTH_STENCIL_WRITE;
    case RhiTextureLayout::DepthStencilRead:  return D3D12_BARRIER_LAYOUT_DEPTH_STENCIL_READ;
    case RhiTextureLayout::CopySource:        return D3D12_BARRIER_LAYOUT_COPY_SOURCE;
    case RhiTextureLayout::CopyDest:          return D3D12_BARRIER_LAYOUT_COPY_DEST;
    case RhiTextureLayout::ResolveSource:     return D3D12_BARRIER_LAYOUT_RESOLVE_SOURCE;
    case RhiTextureLayout::ResolveDest:       return D3D12_BARRIER_LAYOUT_RESOLVE_DEST;
    case RhiTextureLayout::Present:           return D3D12_BARRIER_LAYOUT_PRESENT;
    }
    return D3D12_BARRIER_LAYOUT_COMMON;
}

} // namespace goleta::rhi::d3d12
