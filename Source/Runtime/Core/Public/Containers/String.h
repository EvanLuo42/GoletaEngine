#pragma once

/// @file
/// @brief String<A> and StringView: Rust-shaped owning string and borrowed view.
/// @note  v0 implementation proxies std::basic_string through the allocator adapter. Rust
///        guarantees UTF-8 at the type level; these types do not -- they hold arbitrary bytes.
///        Treat as UTF-8 by convention; find / contains / startsWith / endsWith operate on
///        bytes, not on Unicode scalar values.

#include "Memory/Allocator.h"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <compare>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>

namespace goleta {

using StringView = std::string_view;

/// @brief Owning, growable UTF-8-by-convention string.
/// @tparam A Allocator type. Defaults to goleta::allocators::Global.
template <class A = allocators::Global>
class BasicString
{
private:
    using AllocAdapter = detail::StdAllocatorAdapter<char, A>;
    using StdImpl      = std::basic_string<char, std::char_traits<char>, AllocAdapter>;
    StdImpl Inner;

public:
    using ValueType      = char;
    using AllocatorType  = A;
    using SizeType       = std::size_t;
    using Iterator       = typename StdImpl::iterator;
    using ConstIterator  = typename StdImpl::const_iterator;

    /// @brief The byte offset returned by find() when the needle is absent.
    static constexpr SizeType Npos = static_cast<SizeType>(-1);

    /// @brief Default-construct an empty String.
    BasicString() noexcept = default;

    /// @brief Construct an empty String with the given allocator.
    explicit BasicString(A Alloc) noexcept : Inner(AllocAdapter{std::move(Alloc)}) {}

    /// @brief Construct from a null-terminated C string.
    BasicString(const char* S) : Inner(S ? S : "") {}

    /// @brief Construct from a byte view.
    BasicString(StringView S) : Inner(S.data(), S.size()) {}

    /// @brief Construct from a raw pointer and byte length.
    BasicString(const char* S, SizeType N) : Inner(S, N) {}

    /// @brief Construct N copies of byte C.
    BasicString(SizeType N, char C) : Inner(N, C) {}

    /// @brief Build an empty String with space reserved for at least Cap bytes.
    static BasicString withCapacity(SizeType Cap)
    {
        BasicString S;
        S.Inner.reserve(Cap);
        return S;
    }

    /// @brief Like withCapacity, but with an explicit allocator.
    static BasicString withCapacityIn(SizeType Cap, A Alloc)
    {
        BasicString S{std::move(Alloc)};
        S.Inner.reserve(Cap);
        return S;
    }

    BasicString(const BasicString&)                = default;
    BasicString(BasicString&&) noexcept            = default;
    BasicString& operator=(const BasicString&)     = default;
    BasicString& operator=(BasicString&&) noexcept = default;
    ~BasicString()                                 = default;

    /// @brief Length in bytes.
    SizeType len() const noexcept { return Inner.size(); }

    /// @brief Allocated capacity in bytes (excluding the null terminator).
    SizeType capacity() const noexcept { return Inner.capacity(); }

    /// @brief Whether the length is zero.
    bool isEmpty() const noexcept { return Inner.empty(); }

    /// @brief Reserve room for at least Additional more bytes beyond the current length.
    void reserve(SizeType Additional) { Inner.reserve(Inner.size() + Additional); }

    /// @brief Same as reserve in this implementation. Present to mirror Rust's reserve_exact.
    void reserveExact(SizeType Additional) { Inner.reserve(Inner.size() + Additional); }

    /// @brief Drop excess capacity.
    void shrinkToFit() { Inner.shrink_to_fit(); }

    /// @brief Drop trailing bytes so len() == NewLen. No-op if already smaller.
    void truncate(SizeType NewLen)
    {
        if (NewLen < Inner.size()) Inner.resize(NewLen);
    }

    /// @brief Destroy all bytes; capacity is unchanged.
    void clear() noexcept { Inner.clear(); }

    /// @brief Byte at index I. Asserts in debug if out of bounds.
    char  operator[](SizeType I) const noexcept { return Inner[I]; }
    char& operator[](SizeType I)       noexcept { return Inner[I]; }

    /// @brief Null-terminated C string. Valid until the next mutation.
    const char* cStr() const noexcept { return Inner.c_str(); }

    /// @brief Raw pointer to the byte buffer (null-terminated).
    const char* data() const noexcept { return Inner.data(); }
    char*       data()       noexcept { return Inner.data(); }

    /// @brief View of the bytes as a StringView.
    StringView asStr() const noexcept { return StringView{Inner.data(), Inner.size()}; }

    /// @brief Implicit byte-view conversion.
    operator StringView() const noexcept { return asStr(); }

    /// @brief View of the bytes as a const byte span.
    std::span<const std::uint8_t> asBytes() const noexcept
    {
        return {reinterpret_cast<const std::uint8_t*>(Inner.data()), Inner.size()};
    }

    /// @brief View of the bytes as a mutable byte span.
    std::span<std::uint8_t> asMutBytes() noexcept
    {
        return {reinterpret_cast<std::uint8_t*>(Inner.data()), Inner.size()};
    }

    Iterator      begin()  noexcept       { return Inner.begin(); }
    Iterator      end()    noexcept       { return Inner.end();   }
    ConstIterator begin()  const noexcept { return Inner.begin(); }
    ConstIterator end()    const noexcept { return Inner.end();   }
    ConstIterator cbegin() const noexcept { return Inner.cbegin();}
    ConstIterator cend()   const noexcept { return Inner.cend();  }

    /// @brief Append one byte.
    void push(char C) { Inner.push_back(C); }

    /// @brief Append every byte in S.
    void pushStr(StringView S) { Inner.append(S.data(), S.size()); }

    /// @brief Remove the last byte and return it, or nullopt if empty.
    std::optional<char> pop()
    {
        if (Inner.empty()) return std::nullopt;
        char C = Inner.back();
        Inner.pop_back();
        return C;
    }

    /// @brief Insert byte C at index I. I may equal len().
    void insert(SizeType I, char C) { Inner.insert(Inner.begin() + I, C); }

    /// @brief Insert every byte of S at index I. I may equal len().
    void insertStr(SizeType I, StringView S) { Inner.insert(I, S.data(), S.size()); }

    /// @brief Remove and return the byte at I.
    char remove(SizeType I)
    {
        char C = Inner[I];
        Inner.erase(Inner.begin() + I);
        return C;
    }

    /// @brief Byte offset of the first occurrence of Needle, or Npos if absent.
    SizeType find(StringView Needle) const noexcept
    {
        const auto Pos = Inner.find(Needle.data(), 0, Needle.size());
        return Pos == StdImpl::npos ? Npos : static_cast<SizeType>(Pos);
    }

    /// @brief Byte offset of the first occurrence of C, or Npos if absent.
    SizeType find(char C) const noexcept
    {
        const auto Pos = Inner.find(C);
        return Pos == StdImpl::npos ? Npos : static_cast<SizeType>(Pos);
    }

    /// @brief Whether Needle occurs anywhere within this string.
    bool contains(StringView Needle) const noexcept { return find(Needle) != Npos; }

    /// @brief Whether the string begins with Prefix.
    bool startsWith(StringView Prefix) const noexcept
    {
        return Inner.size() >= Prefix.size()
            && std::memcmp(Inner.data(), Prefix.data(), Prefix.size()) == 0;
    }

    /// @brief Whether the string ends with Suffix.
    bool endsWith(StringView Suffix) const noexcept
    {
        return Inner.size() >= Suffix.size()
            && std::memcmp(Inner.data() + Inner.size() - Suffix.size(),
                           Suffix.data(),
                           Suffix.size()) == 0;
    }

    /// @brief Append S.
    BasicString& operator+=(StringView S) { pushStr(S); return *this; }

    /// @brief Append byte C.
    BasicString& operator+=(char C) { push(C); return *this; }

    /// @brief A copy of the underlying allocator.
    const A& allocator() const noexcept { return Inner.get_allocator().Alloc; }

    /// @brief Non-owning access to the backing std::basic_string. Escape hatch for interop.
    const StdImpl& asStdString() const noexcept { return Inner; }
};

template <class A>
bool operator==(const BasicString<A>& L, const BasicString<A>& R) noexcept
{
    return L.asStr() == R.asStr();
}

template <class A>
bool operator==(const BasicString<A>& L, StringView R) noexcept { return L.asStr() == R; }

template <class A>
bool operator==(StringView L, const BasicString<A>& R) noexcept { return L == R.asStr(); }

template <class A>
bool operator==(const BasicString<A>& L, const char* R) noexcept { return L.asStr() == StringView{R}; }

template <class A>
bool operator==(const char* L, const BasicString<A>& R) noexcept { return StringView{L} == R.asStr(); }

template <class A>
auto operator<=>(const BasicString<A>& L, const BasicString<A>& R) noexcept
{
    return L.asStr() <=> R.asStr();
}

template <class A>
auto operator<=>(const BasicString<A>& L, StringView R) noexcept
{
    return L.asStr() <=> R;
}

template <class A>
BasicString<A> operator+(BasicString<A> L, StringView R)
{
    L.pushStr(R);
    return L;
}

template <class A>
BasicString<A> operator+(StringView L, const BasicString<A>& R)
{
    BasicString<A> Out;
    Out.reserve(L.size() + R.len());
    Out.pushStr(L);
    Out.pushStr(R.asStr());
    return Out;
}

using String = BasicString<>;

} // namespace goleta

namespace std {

template <class A>
struct hash<::goleta::BasicString<A>>
{
    std::size_t operator()(const ::goleta::BasicString<A>& S) const noexcept
    {
        return std::hash<::goleta::StringView>{}(S.asStr());
    }
};

} // namespace std
