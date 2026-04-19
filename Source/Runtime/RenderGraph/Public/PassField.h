#pragma once

/// @file
/// @brief Typed pass-field handles: PassInput<T>, PassOutput<T>, PassInputOutput<T>.
///        Edges between passes are built from these handles, not from string paths.

#include "RenderGraphTypes.h"

namespace goleta::rg
{

/// @brief Tag type marking a field that backs an IRhiTexture.
struct Texture
{
};

/// @brief Tag type marking a field that backs an IRhiBuffer.
struct Buffer
{
};

/// @brief Read-only handle to another pass's field.
/// @tparam T Resource tag (rg::Texture or rg::Buffer); enforced at connect time.
template <class T>
struct PassInput
{
    PassId  Pass  = PassId::Invalid;
    FieldId Field = FieldId::Invalid;

    [[nodiscard]] constexpr bool isValid() const noexcept { return Pass != PassId::Invalid; }
};

/// @brief Write-only handle declared by a pass.
/// @tparam T Resource tag.
template <class T>
struct PassOutput
{
    PassId  Pass  = PassId::Invalid;
    FieldId Field = FieldId::Invalid;

    [[nodiscard]] constexpr bool isValid() const noexcept { return Pass != PassId::Invalid; }
};

/// @brief Read-modify-write handle. Implicitly convertible to either direction so the same
///        field can feed both producer-side and consumer-side connect() calls.
template <class T>
struct PassInputOutput
{
    PassId  Pass  = PassId::Invalid;
    FieldId Field = FieldId::Invalid;

    [[nodiscard]] constexpr bool isValid() const noexcept { return Pass != PassId::Invalid; }

    explicit constexpr operator PassOutput<T>() const noexcept { return PassOutput<T>{Pass, Field}; }
    explicit constexpr operator PassInput<T>() const noexcept { return PassInput<T>{Pass, Field}; }
};

} // namespace goleta::rg
