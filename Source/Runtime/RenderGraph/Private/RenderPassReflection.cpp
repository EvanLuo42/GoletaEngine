#include "RenderPassReflection.h"

#include <cassert>

namespace goleta::rg
{

FieldDesc& RenderPassReflection::Field::desc() noexcept
{
    assert(Owner);
    return Owner->Fields_[static_cast<uint32_t>(Id)];
}

const FieldDesc& RenderPassReflection::Field::desc() const noexcept
{
    assert(Owner);
    return Owner->Fields_[static_cast<uint32_t>(Id)];
}

RenderPassReflection::Field& RenderPassReflection::Field::format(rhi::RhiFormat F) noexcept
{
    desc().Format = F;
    return *this;
}

RenderPassReflection::Field& RenderPassReflection::Field::extent2D(uint32_t W, uint32_t H) noexcept
{
    auto& D = desc();
    D.Width  = W;
    D.Height = H;
    D.Depth  = 1;
    return *this;
}

RenderPassReflection::Field& RenderPassReflection::Field::extent3D(uint32_t W, uint32_t H,
                                                                   uint32_t D) noexcept
{
    auto& Fd = desc();
    Fd.Width  = W;
    Fd.Height = H;
    Fd.Depth  = D;
    return *this;
}

RenderPassReflection::Field& RenderPassReflection::Field::mips(uint32_t N) noexcept
{
    desc().MipLevels = N;
    return *this;
}

RenderPassReflection::Field& RenderPassReflection::Field::arrayLayers(uint32_t N) noexcept
{
    desc().ArrayLayers = N;
    return *this;
}

RenderPassReflection::Field& RenderPassReflection::Field::samples(rhi::RhiSampleCount S) noexcept
{
    desc().Samples = S;
    return *this;
}

RenderPassReflection::Field& RenderPassReflection::Field::usage(rhi::RhiTextureUsage U) noexcept
{
    desc().TextureUsage = U;
    return *this;
}

RenderPassReflection::Field& RenderPassReflection::Field::bindAs(RgBindAs B) noexcept
{
    desc().BindAs = B;
    return *this;
}

RenderPassReflection::Field& RenderPassReflection::Field::sizeBytes(uint64_t S) noexcept
{
    auto& D = desc();
    D.ResourceType = RgResourceType::Buffer;
    D.SizeBytes    = S;
    return *this;
}

RenderPassReflection::Field& RenderPassReflection::Field::structStride(uint32_t B) noexcept
{
    auto& D = desc();
    D.ResourceType    = RgResourceType::Buffer;
    D.StructureStride = B;
    return *this;
}

RenderPassReflection::Field& RenderPassReflection::Field::bufferUsage(rhi::RhiBufferUsage U) noexcept
{
    auto& D = desc();
    D.ResourceType = RgResourceType::Buffer;
    D.BufferUsage  = U;
    return *this;
}

RenderPassReflection::Field& RenderPassReflection::Field::persistent() noexcept
{
    desc().Lifetime = RgLifetime::Persistent;
    return *this;
}

RenderPassReflection::Field RenderPassReflection::addField(std::string_view Name, RgAccessMode Access)
{
    FieldDesc D;
    D.Name       = std::string(Name);
    D.AccessMode = Access;
    const FieldId Id = static_cast<FieldId>(static_cast<uint32_t>(Fields_.size()));
    Fields_.emplace_back(std::move(D));
    ByName_.emplace(std::string(Name), Id);
    return Field{this, OwnerPass_, Id};
}

RenderPassReflection::Field RenderPassReflection::addInput(std::string_view Name)
{
    return addField(Name, RgAccessMode::Input);
}

RenderPassReflection::Field RenderPassReflection::addOutput(std::string_view Name)
{
    return addField(Name, RgAccessMode::Output);
}

RenderPassReflection::Field RenderPassReflection::addInputOutput(std::string_view Name)
{
    return addField(Name, RgAccessMode::InputOutput);
}

std::span<const FieldDesc> RenderPassReflection::fields() const noexcept
{
    return {Fields_.data(), Fields_.size()};
}

PassId RenderPassReflection::ownerPass() const noexcept
{
    return OwnerPass_;
}

FieldId RenderPassReflection::findByName(std::string_view Name) const noexcept
{
    auto It = ByName_.find(std::string(Name));
    return It == ByName_.end() ? FieldId::Invalid : It->second;
}

} // namespace goleta::rg
