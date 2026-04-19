#include "ResourcePool.h"

#include "RHIDevice.h"

namespace goleta::rg
{

namespace
{

uint64_t mix(uint64_t H, uint64_t V) noexcept
{
    H ^= V + 0x9E3779B97F4A7C15ull + (H << 6) + (H >> 2);
    return H;
}

} // namespace

uint64_t ResourcePool::hashTextureDesc(const rhi::RhiTextureDesc& Desc) noexcept
{
    uint64_t H = 0xCBF29CE484222325ull;
    H = mix(H, static_cast<uint64_t>(Desc.Dimension));
    H = mix(H, static_cast<uint64_t>(Desc.Format));
    H = mix(H, Desc.Width);
    H = mix(H, Desc.Height);
    H = mix(H, Desc.DepthOrArrayLayers);
    H = mix(H, Desc.MipLevels);
    H = mix(H, static_cast<uint64_t>(Desc.Samples));
    H = mix(H, static_cast<uint64_t>(Desc.Usage));
    H = mix(H, static_cast<uint64_t>(Desc.Location));
    return H;
}

uint64_t ResourcePool::hashBufferDesc(const rhi::RhiBufferDesc& Desc) noexcept
{
    uint64_t H = 0xCBF29CE484222325ull;
    H = mix(H, Desc.SizeBytes);
    H = mix(H, static_cast<uint64_t>(Desc.Usage));
    H = mix(H, static_cast<uint64_t>(Desc.Location));
    H = mix(H, Desc.StructureStride);
    return H;
}

Rc<rhi::IRhiTexture> ResourcePool::acquireTexture(rhi::IRhiDevice&           Device,
                                                  const rhi::RhiTextureDesc& Desc)
{
    const uint64_t Key = hashTextureDesc(Desc);
    auto It = Textures_.find(Key);
    if (It != Textures_.end() && !It->second.empty())
    {
        Rc<rhi::IRhiTexture> Tex = std::move(It->second.back());
        It->second.pop_back();
        ++TextureHits_;
        return Tex;
    }
    return Device.createTexture(Desc);
}

Rc<rhi::IRhiBuffer> ResourcePool::acquireBuffer(rhi::IRhiDevice&          Device,
                                                const rhi::RhiBufferDesc& Desc)
{
    const uint64_t Key = hashBufferDesc(Desc);
    auto It = Buffers_.find(Key);
    if (It != Buffers_.end() && !It->second.empty())
    {
        Rc<rhi::IRhiBuffer> Buf = std::move(It->second.back());
        It->second.pop_back();
        ++BufferHits_;
        return Buf;
    }
    return Device.createBuffer(Desc);
}

void ResourcePool::releaseTexture(Rc<rhi::IRhiTexture>       Tex,
                                  const rhi::RhiTextureDesc& Desc)
{
    if (!Tex)
        return;
    Textures_[hashTextureDesc(Desc)].push_back(std::move(Tex));
}

void ResourcePool::releaseBuffer(Rc<rhi::IRhiBuffer>       Buf,
                                 const rhi::RhiBufferDesc& Desc)
{
    if (!Buf)
        return;
    Buffers_[hashBufferDesc(Desc)].push_back(std::move(Buf));
}

void ResourcePool::clear() noexcept
{
    Textures_.clear();
    Buffers_.clear();
}

} // namespace goleta::rg
