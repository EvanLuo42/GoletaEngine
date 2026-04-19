#include "RenderData.h"

#include <cassert>

#include "CompiledGraph.h"
#include "RHIBuffer.h"
#include "RHITexture.h"
#include "ResourceRegistry.h"

namespace goleta::rg
{

rhi::IRhiTexture* RenderData::resolveTexture(PassId Pass, FieldId Field) const noexcept
{
    if (Compiled_ == nullptr || Registry_ == nullptr)
        return nullptr;
    assert(Pass == CurrentPass_ && "PassField handle belongs to a different pass");
    const uint32_t Pi = static_cast<uint32_t>(Pass);
    const uint32_t Fi = static_cast<uint32_t>(Field);
    if (Pi >= Compiled_->FieldToLogical.size())
        return nullptr;
    const auto& Row = Compiled_->FieldToLogical[Pi];
    if (Fi >= Row.size())
        return nullptr;
    return Registry_->texture(Row[Fi]);
}

rhi::IRhiBuffer* RenderData::resolveBuffer(PassId Pass, FieldId Field) const noexcept
{
    if (Compiled_ == nullptr || Registry_ == nullptr)
        return nullptr;
    assert(Pass == CurrentPass_ && "PassField handle belongs to a different pass");
    const uint32_t Pi = static_cast<uint32_t>(Pass);
    const uint32_t Fi = static_cast<uint32_t>(Field);
    if (Pi >= Compiled_->FieldToLogical.size())
        return nullptr;
    const auto& Row = Compiled_->FieldToLogical[Pi];
    if (Fi >= Row.size())
        return nullptr;
    return Registry_->buffer(Row[Fi]);
}

rhi::IRhiTexture* RenderData::getTexture(PassInput<Texture> H) const noexcept
{
    return resolveTexture(H.Pass, H.Field);
}

rhi::IRhiTexture* RenderData::getTexture(PassOutput<Texture> H) const noexcept
{
    return resolveTexture(H.Pass, H.Field);
}

rhi::IRhiTexture* RenderData::getTexture(PassInputOutput<Texture> H) const noexcept
{
    return resolveTexture(H.Pass, H.Field);
}

rhi::IRhiBuffer* RenderData::getBuffer(PassInput<Buffer> H) const noexcept
{
    return resolveBuffer(H.Pass, H.Field);
}

rhi::IRhiBuffer* RenderData::getBuffer(PassOutput<Buffer> H) const noexcept
{
    return resolveBuffer(H.Pass, H.Field);
}

rhi::IRhiBuffer* RenderData::getBuffer(PassInputOutput<Buffer> H) const noexcept
{
    return resolveBuffer(H.Pass, H.Field);
}

rhi::RhiTextureHandle RenderData::getTextureSrv(PassInput<Texture> H) const noexcept
{
    rhi::IRhiTexture* Tex = resolveTexture(H.Pass, H.Field);
    return Tex ? Tex->srvHandle() : rhi::RhiTextureHandle{};
}

rhi::RhiRwTextureHandle RenderData::getTextureUav(PassOutput<Texture> H) const noexcept
{
    rhi::IRhiTexture* Tex = resolveTexture(H.Pass, H.Field);
    return Tex ? Tex->uavHandle() : rhi::RhiRwTextureHandle{};
}

rhi::RhiRwTextureHandle RenderData::getTextureUav(PassInputOutput<Texture> H) const noexcept
{
    rhi::IRhiTexture* Tex = resolveTexture(H.Pass, H.Field);
    return Tex ? Tex->uavHandle() : rhi::RhiRwTextureHandle{};
}

} // namespace goleta::rg
