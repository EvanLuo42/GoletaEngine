#pragma once

/// @file
/// @brief Compile-time 64-bit type identity via compiler pretty-function-name hashing. Used
///        where std::type_index would be, without the RTTI dependency.

#include <cstdint>
#include <string_view>

namespace goleta
{

namespace detail
{

/// @brief Raw compiler-specific string that mentions T. Format varies per compiler but is
///        stable within a single build: same T always produces the same bytes.
template <class T>
constexpr std::string_view rawTypeName() noexcept
{
#if defined(_MSC_VER) && !defined(__clang__)
    return {__FUNCSIG__, sizeof(__FUNCSIG__) - 1};
#elif defined(__clang__) || defined(__GNUC__)
    return {__PRETTY_FUNCTION__, sizeof(__PRETTY_FUNCTION__) - 1};
#else
#    error "goleta::detail::rawTypeName: unsupported compiler"
#endif
}

/// @brief 64-bit FNV-1a hash. Good enough for type-name uniqueness; collision probability on
///        realistic type-name strings is astronomically small.
constexpr uint64_t fnv1a64(std::string_view Bytes) noexcept
{
    uint64_t Hash = 0xcbf29ce484222325ull;
    for (char C : Bytes)
    {
        Hash ^= static_cast<uint8_t>(C);
        Hash *= 0x100000001b3ull;
    }
    return Hash;
}

} // namespace detail

/// @brief 64-bit compile-time identity for a type. Stable across DSOs that share the same
///        compiler version -- the hash is over a compiler-emitted string containing T, which
///        is produced identically in every translation unit.
/// @note  Not stable across compilers / compiler versions / different language modes. Use
///        only for in-process identity, never for persisted artifacts.
template <class T>
constexpr uint64_t typeHash() noexcept
{
    return detail::fnv1a64(detail::rawTypeName<T>());
}

} // namespace goleta
