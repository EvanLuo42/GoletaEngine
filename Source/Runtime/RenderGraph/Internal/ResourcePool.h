#pragma once

/// @file
/// @brief Frame-to-frame pool for transient textures and buffers. Phase 1: pooled by descriptor
///        hash with no intra-frame aliasing. Each compile() run acquires fresh physicals.

#include <cstdint>
#include <unordered_map>
#include <vector>

#include "Memory/Rc.h"
#include "RHIBuffer.h"
#include "RHITexture.h"

namespace goleta::rhi
{
class IRhiDevice;
}

namespace goleta::rg
{

/// @brief Pool keyed by RhiTextureDesc / RhiBufferDesc. Resources released to the pool are
///        kept alive across compile() cycles so the next similar allocation is free of
///        driver-side work.
class ResourcePool
{
public:
    Rc<rhi::IRhiTexture> acquireTexture(rhi::IRhiDevice& Device, const rhi::RhiTextureDesc& Desc);
    Rc<rhi::IRhiBuffer>  acquireBuffer(rhi::IRhiDevice& Device, const rhi::RhiBufferDesc& Desc);

    void releaseTexture(Rc<rhi::IRhiTexture> Tex, const rhi::RhiTextureDesc& Desc);
    void releaseBuffer(Rc<rhi::IRhiBuffer> Buf, const rhi::RhiBufferDesc& Desc);

    /// @brief Drop every pooled resource. Invoked at shutdown or when the device changes.
    void clear() noexcept;

    [[nodiscard]] uint64_t textureHitCount() const noexcept { return TextureHits_; }
    [[nodiscard]] uint64_t bufferHitCount() const noexcept { return BufferHits_; }

private:
    static uint64_t hashTextureDesc(const rhi::RhiTextureDesc& Desc) noexcept;
    static uint64_t hashBufferDesc(const rhi::RhiBufferDesc& Desc) noexcept;

    std::unordered_map<uint64_t, std::vector<Rc<rhi::IRhiTexture>>> Textures_;
    std::unordered_map<uint64_t, std::vector<Rc<rhi::IRhiBuffer>>>  Buffers_;
    uint64_t                                                        TextureHits_ = 0;
    uint64_t                                                        BufferHits_  = 0;
};

} // namespace goleta::rg
