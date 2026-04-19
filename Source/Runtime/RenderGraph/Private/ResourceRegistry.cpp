#include "ResourceRegistry.h"

#include "CompiledGraph.h"
#include "RHIDevice.h"
#include "ResourcePool.h"

namespace goleta::rg
{

void ResourceRegistry::beginFrame(rhi::IRhiDevice& /*Device*/, const CompiledGraph& Compiled,
                                  ResourcePool& /*Pool*/)
{
    const size_t N = Compiled.LogicalResources.size();
    Textures_.assign(N, Rc<rhi::IRhiTexture>{});
    Buffers_.assign(N, Rc<rhi::IRhiBuffer>{});

    for (size_t i = 0; i < N; ++i)
    {
        const auto& R = Compiled.LogicalResources[i];
        if (R.Lifetime != RgLifetime::Imported)
            continue;
        if (R.ResourceType == RgResourceType::Texture)
            Textures_[i] = R.ImportedTexture;
        else
            Buffers_[i] = R.ImportedBuffer;
    }
}

void ResourceRegistry::acquireForGroup(rhi::IRhiDevice& Device, const CompiledGraph& Compiled,
                                       const std::vector<uint32_t>& Logicals, ResourcePool& Pool)
{
    for (uint32_t lid : Logicals)
    {
        if (lid >= Compiled.LogicalResources.size())
            continue;
        const auto& R = Compiled.LogicalResources[lid];
        if (R.Lifetime == RgLifetime::Imported)
            continue;
        if (R.ResourceType == RgResourceType::Texture)
        {
            if (Textures_[lid])
                continue;
            rhi::RhiTextureDesc Desc = R.TextureDesc;
            if (!R.DebugName.empty())
                Desc.DebugName = R.DebugName.c_str();
            Textures_[lid] = Pool.acquireTexture(Device, Desc);
            if (Textures_[lid] && !R.DebugName.empty())
                Textures_[lid]->setDebugName(R.DebugName.c_str());
        }
        else
        {
            if (Buffers_[lid])
                continue;
            rhi::RhiBufferDesc Desc = R.BufferDesc;
            if (!R.DebugName.empty())
                Desc.DebugName = R.DebugName.c_str();
            Buffers_[lid] = Pool.acquireBuffer(Device, Desc);
            if (Buffers_[lid] && !R.DebugName.empty())
                Buffers_[lid]->setDebugName(R.DebugName.c_str());
        }
    }
}

void ResourceRegistry::releaseAfterGroup(const CompiledGraph&         Compiled,
                                         const std::vector<uint32_t>& Logicals,
                                         ResourcePool&                Pool)
{
    for (uint32_t lid : Logicals)
    {
        if (lid >= Compiled.LogicalResources.size())
            continue;
        const auto& R = Compiled.LogicalResources[lid];
        if (R.Lifetime != RgLifetime::Transient)
            continue;
        if (R.ResourceType == RgResourceType::Texture && Textures_[lid])
            Pool.releaseTexture(std::move(Textures_[lid]), R.TextureDesc);
        else if (R.ResourceType == RgResourceType::Buffer && Buffers_[lid])
            Pool.releaseBuffer(std::move(Buffers_[lid]), R.BufferDesc);
    }
}

void ResourceRegistry::endFrame(const CompiledGraph& Compiled, ResourcePool& Pool) noexcept
{
    for (size_t i = 0; i < Compiled.LogicalResources.size(); ++i)
    {
        const auto& R = Compiled.LogicalResources[i];
        if (R.Lifetime != RgLifetime::Transient)
            continue;
        if (R.ResourceType == RgResourceType::Texture && i < Textures_.size() && Textures_[i])
            Pool.releaseTexture(std::move(Textures_[i]), R.TextureDesc);
        else if (R.ResourceType == RgResourceType::Buffer && i < Buffers_.size() && Buffers_[i])
            Pool.releaseBuffer(std::move(Buffers_[i]), R.BufferDesc);
    }
    Textures_.clear();
    Buffers_.clear();
}

rhi::IRhiTexture* ResourceRegistry::texture(LogicalResourceId Id) const noexcept
{
    const uint32_t Idx = static_cast<uint32_t>(Id);
    if (Idx >= Textures_.size())
        return nullptr;
    return Textures_[Idx].get();
}

rhi::IRhiBuffer* ResourceRegistry::buffer(LogicalResourceId Id) const noexcept
{
    const uint32_t Idx = static_cast<uint32_t>(Id);
    if (Idx >= Buffers_.size())
        return nullptr;
    return Buffers_[Idx].get();
}

} // namespace goleta::rg
