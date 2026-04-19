#pragma once

/// @file
/// @brief Typed bindless indices used when pushing descriptor references through root constants.

#include <cstdint>
#include <functional>

namespace goleta::rhi
{

/// @brief Sentinel for "no descriptor". Always 0, so default-constructed handles are invalid.
inline constexpr uint32_t InvalidBindlessIndex = 0u;

/// @brief Untyped bindless descriptor index. Zero is always invalid.
struct RhiBindlessIndex
{
    uint32_t Value = InvalidBindlessIndex;

    constexpr bool isValid() const noexcept { return Value != InvalidBindlessIndex; }

    constexpr explicit operator bool() const noexcept { return isValid(); }

    friend constexpr bool operator==(const RhiBindlessIndex A, const RhiBindlessIndex B) noexcept
    {
        return A.Value == B.Value;
    }
    friend constexpr bool operator!=(const RhiBindlessIndex A, const RhiBindlessIndex B) noexcept
    {
        return A.Value != B.Value;
    }
};

/// @brief Typed bindless wrapper. Only the index is stored; resolved by the shader via
///        ResourceDescriptorHeap[idx] / HLSL 6.6 dynamic resources / Vulkan descriptor indexing.
/// @tparam Tag  Empty phantom tag type — distinct per resource kind for type safety.
template <class Tag>
struct RhiTypedBindless
{
    uint32_t Index = InvalidBindlessIndex;

    constexpr bool isValid() const noexcept { return Index != InvalidBindlessIndex; }

    constexpr explicit operator bool() const noexcept { return isValid(); }

    friend constexpr bool operator==(RhiTypedBindless A, RhiTypedBindless B) noexcept { return A.Index == B.Index; }
    friend constexpr bool operator!=(RhiTypedBindless A, RhiTypedBindless B) noexcept { return A.Index != B.Index; }
};

struct RhiBufferTag
{
};
struct RhiRwBufferTag
{
};
struct RhiTextureTag
{
};
struct RhiRwTextureTag
{
};
struct RhiSamplerTag
{
};
struct RhiAccelStructTag
{
};

using RhiBufferHandle      = RhiTypedBindless<RhiBufferTag>;
using RhiRwBufferHandle    = RhiTypedBindless<RhiRwBufferTag>;
using RhiTextureHandle     = RhiTypedBindless<RhiTextureTag>;
using RhiRwTextureHandle   = RhiTypedBindless<RhiRwTextureTag>;
using RhiSamplerHandle     = RhiTypedBindless<RhiSamplerTag>;
using RhiAccelStructHandle = RhiTypedBindless<RhiAccelStructTag>;

} // namespace goleta::rhi

namespace std
{
template <>
struct hash<goleta::rhi::RhiBindlessIndex>
{
    size_t operator()(const goleta::rhi::RhiBindlessIndex Key) const noexcept
    {
        return std::hash<uint32_t>{}(Key.Value);
    }
};

template <class Tag>
struct hash<goleta::rhi::RhiTypedBindless<Tag>>
{
    size_t operator()(goleta::rhi::RhiTypedBindless<Tag> Key) const noexcept
    {
        return std::hash<uint32_t>{}(Key.Index);
    }
};
} // namespace std
