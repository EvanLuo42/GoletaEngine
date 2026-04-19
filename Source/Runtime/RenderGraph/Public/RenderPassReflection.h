#pragma once

/// @file
/// @brief RenderPassReflection: how a RenderPass declares its inputs, outputs, and inouts.
///        The Field builder returned by addInput/addOutput/addInputOutput is chainable and
///        convertible to the pass's typed PassInput/PassOutput/PassInputOutput members.

#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <vector>

#include "PassField.h"
#include "RHIBarrier.h"
#include "RHIEnums.h"
#include "RHIFormat.h"
#include "RenderGraphExport.h"
#include "RenderGraphTypes.h"

namespace goleta::rg
{

/// @brief Captured metadata for one reflected field.
struct FieldDesc
{
    std::string          Name;
    RgResourceType       ResourceType = RgResourceType::Texture;
    RgAccessMode         AccessMode   = RgAccessMode::Input;
    RgBindAs             BindAs       = RgBindAs::Auto;

    rhi::RhiFormat       Format       = rhi::RhiFormat::Unknown;
    uint32_t             Width        = 0;
    uint32_t             Height       = 0;
    uint32_t             Depth        = 1;
    uint32_t             MipLevels    = 1;
    uint32_t             ArrayLayers  = 1;
    rhi::RhiTextureUsage TextureUsage = rhi::RhiTextureUsage::None;
    rhi::RhiSampleCount  Samples      = rhi::RhiSampleCount::X1;

    uint64_t             SizeBytes       = 0;
    uint32_t             StructureStride = 0;
    rhi::RhiBufferUsage  BufferUsage     = rhi::RhiBufferUsage::None;

    RgLifetime           Lifetime = RgLifetime::Transient;
};

/// @brief Per-pass reflection object. Each RenderPass populates one of these in reflect().
class RENDERGRAPH_API RenderPassReflection
{
public:
    /// @brief Chainable builder for a single reflected field. Setters return *this. Assigning
    ///        the Field to a PassInput<T> / PassOutput<T> / PassInputOutput<T> member seals
    ///        the {PassId, FieldId} pair on that handle.
    class RENDERGRAPH_API Field
    {
    public:
        Field& format(rhi::RhiFormat F) noexcept;
        Field& extent2D(uint32_t W, uint32_t H) noexcept;
        Field& extent3D(uint32_t W, uint32_t H, uint32_t D) noexcept;
        Field& mips(uint32_t N) noexcept;
        Field& arrayLayers(uint32_t N) noexcept;
        Field& samples(rhi::RhiSampleCount S) noexcept;
        Field& usage(rhi::RhiTextureUsage U) noexcept;
        Field& bindAs(RgBindAs B) noexcept;
        Field& sizeBytes(uint64_t S) noexcept;
        Field& structStride(uint32_t B) noexcept;
        Field& bufferUsage(rhi::RhiBufferUsage U) noexcept;
        Field& persistent() noexcept;

        /// @brief Convert to a PassInput<T>. Static-asserts on tag/direction mismatch.
        template <class T>
        operator PassInput<T>() const noexcept;

        /// @brief Convert to a PassOutput<T>. Static-asserts on tag/direction mismatch.
        template <class T>
        operator PassOutput<T>() const noexcept;

        /// @brief Convert to a PassInputOutput<T>. Static-asserts if the field isn't InputOutput.
        template <class T>
        operator PassInputOutput<T>() const noexcept;

    private:
        friend class RenderPassReflection;
        Field(RenderPassReflection* OwnerIn, PassId PassIn, FieldId FieldIn) noexcept
            : Owner(OwnerIn), OwnerPass(PassIn), Id(FieldIn)
        {
        }

        FieldDesc&       desc() noexcept;
        const FieldDesc& desc() const noexcept;

        RenderPassReflection* Owner     = nullptr;
        PassId                OwnerPass = PassId::Invalid;
        FieldId               Id        = FieldId::Invalid;
    };

    /// @brief Add an input field (read-only).
    Field addInput(std::string_view Name);

    /// @brief Add an output field (write-only; discards prior content).
    Field addOutput(std::string_view Name);

    /// @brief Add a read-modify-write field.
    Field addInputOutput(std::string_view Name);

    /// @brief Captured metadata; ordering matches FieldId.
    [[nodiscard]] std::span<const FieldDesc> fields() const noexcept;

    /// @brief Owning pass.
    [[nodiscard]] PassId ownerPass() const noexcept;

    /// @brief Look up a field's index by name. Returns FieldId::Invalid when absent.
    [[nodiscard]] FieldId findByName(std::string_view Name) const noexcept;

private:
    friend class RenderGraph;
    explicit RenderPassReflection(PassId Owner) noexcept : OwnerPass_(Owner) {}

    Field addField(std::string_view Name, RgAccessMode Access);

    PassId                                   OwnerPass_ = PassId::Invalid;
    std::vector<FieldDesc>                   Fields_;
    std::unordered_map<std::string, FieldId> ByName_;
};

template <class T>
inline RenderPassReflection::Field::operator PassInput<T>() const noexcept
{
    static_assert(std::is_same_v<T, Texture> || std::is_same_v<T, Buffer>,
                  "PassInput<T> requires T to be rg::Texture or rg::Buffer");
    return PassInput<T>{OwnerPass, Id};
}

template <class T>
inline RenderPassReflection::Field::operator PassOutput<T>() const noexcept
{
    static_assert(std::is_same_v<T, Texture> || std::is_same_v<T, Buffer>,
                  "PassOutput<T> requires T to be rg::Texture or rg::Buffer");
    return PassOutput<T>{OwnerPass, Id};
}

template <class T>
inline RenderPassReflection::Field::operator PassInputOutput<T>() const noexcept
{
    static_assert(std::is_same_v<T, Texture> || std::is_same_v<T, Buffer>,
                  "PassInputOutput<T> requires T to be rg::Texture or rg::Buffer");
    return PassInputOutput<T>{OwnerPass, Id};
}

} // namespace goleta::rg
