#pragma once

/// @file
/// @brief Vec<T, A>: contiguous growable array with Rust-shaped API.
/// @note  v0 implementation proxies std::vector through an allocator adapter.
///        Swapping the backing store later is a non-breaking change -- call sites only touch Vec.

#include "Memory/Allocator.h"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <initializer_list>
#include <iterator>
#include <optional>
#include <span>
#include <type_traits>
#include <utility>
#include <vector>

namespace goleta {

/// @brief Dynamic array with contiguous storage.
/// @tparam T Element type.
/// @tparam A Allocator type. Defaults to goleta::allocators::Global (stateless, aligned new/delete).
template <class T, class A = allocators::Global>
class Vec
{
private:
    using AllocAdapter = detail::StdAllocatorAdapter<T, A>;
    using StdImpl      = std::vector<T, AllocAdapter>;
    StdImpl Inner;

public:
    using ValueType            = T;
    using AllocatorType        = A;
    using SizeType             = std::size_t;
    using DifferenceType       = std::ptrdiff_t;
    using Reference            = T&;
    using ConstReference       = const T&;
    using Pointer              = T*;
    using ConstPointer         = const T*;
    using Iterator             = typename StdImpl::iterator;
    using ConstIterator        = typename StdImpl::const_iterator;
    using ReverseIterator      = typename StdImpl::reverse_iterator;
    using ConstReverseIterator = typename StdImpl::const_reverse_iterator;

    /// @brief Construct an empty Vec with the default-constructed allocator.
    Vec() noexcept = default;

    /// @brief Construct an empty Vec with the given allocator.
    explicit Vec(A Alloc) noexcept : Inner(AllocAdapter{std::move(Alloc)}) {}

    /// @brief Default-construct Count elements.
    explicit Vec(SizeType Count) : Inner(Count) {}

    /// @brief Construct Count copies of Val.
    Vec(SizeType Count, const T& Val) : Inner(Count, Val) {}

    /// @brief Construct from an iterator range [First, Last).
    template <class It, class = std::enable_if_t<!std::is_integral_v<It>>>
    Vec(It First, It Last) : Inner(First, Last) {}

    /// @brief Construct from a braced initializer list.
    Vec(std::initializer_list<T> IL) : Inner(IL) {}

    /// @brief Build an empty Vec with space reserved for at least Cap elements.
    static Vec withCapacity(SizeType Cap)
    {
        Vec V;
        V.Inner.reserve(Cap);
        return V;
    }

    /// @brief Like withCapacity, but with an explicit allocator.
    static Vec withCapacityIn(SizeType Cap, A Alloc)
    {
        Vec V{std::move(Alloc)};
        V.Inner.reserve(Cap);
        return V;
    }

    Vec(const Vec&)                = default;
    Vec(Vec&&) noexcept            = default;
    Vec& operator=(const Vec&)     = default;
    Vec& operator=(Vec&&) noexcept = default;
    ~Vec()                         = default;

    /// @brief Number of elements currently stored.
    SizeType len() const noexcept { return Inner.size(); }

    /// @brief Allocated capacity in elements.
    SizeType capacity() const noexcept { return Inner.capacity(); }

    /// @brief Whether the Vec has zero elements.
    bool isEmpty() const noexcept { return Inner.empty(); }

    /// @brief Reserve room for at least Additional more elements beyond the current len.
    void reserve(SizeType Additional) { Inner.reserve(Inner.size() + Additional); }

    /// @brief Same as reserve in this implementation. Present to mirror Rust's reserve_exact.
    void reserveExact(SizeType Additional) { Inner.reserve(Inner.size() + Additional); }

    /// @brief Drop excess capacity down to len().
    void shrinkToFit() { Inner.shrink_to_fit(); }

    /// @brief Shrink capacity to at least MinCap, never below len().
    void shrinkTo(SizeType MinCap)
    {
        const SizeType Target = MinCap < Inner.size() ? Inner.size() : MinCap;
        if (Target >= Inner.capacity()) return;
        StdImpl Tmp(Inner.get_allocator());
        Tmp.reserve(Target);
        Tmp.insert(Tmp.end(),
                   std::make_move_iterator(Inner.begin()),
                   std::make_move_iterator(Inner.end()));
        Inner = std::move(Tmp);
    }

    /// @brief Drop trailing elements so len() == NewLen. No-op if already smaller.
    void truncate(SizeType NewLen)
    {
        if (NewLen < Inner.size()) Inner.resize(NewLen);
    }

    /// @brief Indexed access. Asserts in debug if I is out of bounds.
    T&       operator[](SizeType I)       { assert(I < Inner.size() && "Vec index out of bounds"); return Inner[I]; }
    const T& operator[](SizeType I) const { assert(I < Inner.size() && "Vec index out of bounds"); return Inner[I]; }

    /// @brief Bounds-checked access. Returns nullptr if I is out of bounds.
    T*       get(SizeType I) noexcept       { return I < Inner.size() ? std::addressof(Inner[I]) : nullptr; }
    const T* get(SizeType I) const noexcept { return I < Inner.size() ? std::addressof(Inner[I]) : nullptr; }

    /// @brief Mutable bounds-checked access. Mirrors Rust's get_mut.
    T* getMut(SizeType I) noexcept { return get(I); }

    /// @brief Pointer to the first element, or nullptr if empty.
    T*       first() noexcept       { return Inner.empty() ? nullptr : std::addressof(Inner.front()); }
    const T* first() const noexcept { return Inner.empty() ? nullptr : std::addressof(Inner.front()); }

    /// @brief Pointer to the last element, or nullptr if empty.
    T*       last() noexcept       { return Inner.empty() ? nullptr : std::addressof(Inner.back()); }
    const T* last() const noexcept { return Inner.empty() ? nullptr : std::addressof(Inner.back()); }

    /// @brief View over all elements as a mutable slice.
    std::span<T>       asMutSlice() noexcept     { return {Inner.data(), Inner.size()}; }

    /// @brief View over all elements as a const slice.
    std::span<const T> asSlice()    const noexcept { return {Inner.data(), Inner.size()}; }

    /// @brief Raw pointer to the underlying buffer. Null if the Vec has never allocated.
    T*       data() noexcept       { return Inner.data(); }
    const T* data() const noexcept { return Inner.data(); }

    Iterator             begin()  noexcept       { return Inner.begin(); }
    Iterator             end()    noexcept       { return Inner.end();   }
    ConstIterator        begin()  const noexcept { return Inner.begin(); }
    ConstIterator        end()    const noexcept { return Inner.end();   }
    ConstIterator        cbegin() const noexcept { return Inner.cbegin();}
    ConstIterator        cend()   const noexcept { return Inner.cend();  }
    ReverseIterator      rbegin() noexcept       { return Inner.rbegin();}
    ReverseIterator      rend()   noexcept       { return Inner.rend();  }
    ConstReverseIterator rbegin() const noexcept { return Inner.rbegin();}
    ConstReverseIterator rend()   const noexcept { return Inner.rend();  }

    /// @brief Const view usable in range-for. Alias of asSlice().
    /// @note  Unlike Rust's lazy iterator, this returns a contiguous span. Semantically close
    ///        enough for typical use; not a drop-in for Rust's iterator adapter chains.
    std::span<const T> iter()    const noexcept { return asSlice(); }

    /// @brief Mutable view usable in range-for. Alias of asMutSlice().
    std::span<T>       iterMut() noexcept       { return asMutSlice(); }

    /// @brief Append a copy of Val to the end.
    void push(const T& Val) { Inner.push_back(Val); }

    /// @brief Move-append Val to the end.
    void push(T&& Val) { Inner.push_back(std::move(Val)); }

    /// @brief Construct a T at the end in place and return a reference to it.
    template <class... Args>
    T& emplace(Args&&... ArgsIn) { return Inner.emplace_back(std::forward<Args>(ArgsIn)...); }

    /// @brief Remove the last element and return it, or nullopt if empty.
    std::optional<T> pop()
    {
        if (Inner.empty()) return std::nullopt;
        std::optional<T> Out{std::move(Inner.back())};
        Inner.pop_back();
        return Out;
    }

    /// @brief Insert Val at index I, shifting later elements right. I may equal len().
    void insert(SizeType I, const T& Val)
    {
        assert(I <= Inner.size() && "Vec insert index out of bounds");
        Inner.insert(Inner.begin() + I, Val);
    }
    void insert(SizeType I, T&& Val)
    {
        assert(I <= Inner.size() && "Vec insert index out of bounds");
        Inner.insert(Inner.begin() + I, std::move(Val));
    }

    /// @brief Remove and return the element at I, shifting later elements left.
    T remove(SizeType I)
    {
        assert(I < Inner.size() && "Vec remove index out of bounds");
        T Out = std::move(Inner[I]);
        Inner.erase(Inner.begin() + I);
        return Out;
    }

    /// @brief Remove and return element at I in O(1) by swapping in the last element.
    /// @note  Reorders the Vec; use remove() to preserve order.
    T swapRemove(SizeType I)
    {
        assert(I < Inner.size() && "Vec swapRemove index out of bounds");
        T Out = std::move(Inner[I]);
        if (I + 1 != Inner.size()) Inner[I] = std::move(Inner.back());
        Inner.pop_back();
        return Out;
    }

    /// @brief Destroy all elements; capacity is unchanged.
    void clear() noexcept { Inner.clear(); }

    /// @brief Resize to NewLen, default-constructing any new elements.
    void resize(SizeType NewLen) { Inner.resize(NewLen); }

    /// @brief Resize to NewLen, copying Val into any new elements.
    void resize(SizeType NewLen, const T& Val) { Inner.resize(NewLen, Val); }

    /// @brief Resize to NewLen, calling MakeValue() for each new element.
    template <class F>
    void resizeWith(SizeType NewLen, F MakeValue)
    {
        while (Inner.size() > NewLen) Inner.pop_back();
        while (Inner.size() < NewLen) Inner.push_back(MakeValue());
    }

    /// @brief Overwrite every element with a copy of Val.
    void fill(const T& Val) { std::fill(Inner.begin(), Inner.end(), Val); }

    /// @brief Overwrite every element with MakeValue().
    template <class F>
    void fillWith(F MakeValue)
    {
        for (auto& E : Inner) E = MakeValue();
    }

    /// @brief Move every element from Other into the end of *this; Other becomes empty.
    void append(Vec& Other)
    {
        Inner.insert(Inner.end(),
                     std::make_move_iterator(Other.Inner.begin()),
                     std::make_move_iterator(Other.Inner.end()));
        Other.Inner.clear();
    }

    /// @brief Copy every element from Slice to the end.
    void extendFromSlice(std::span<const T> Slice)
    {
        Inner.insert(Inner.end(), Slice.begin(), Slice.end());
    }

    /// @brief Copy every element from [First, Last) to the end.
    template <class It>
    void extend(It First, It Last) { Inner.insert(Inner.end(), First, Last); }

    /// @brief Keep only elements for which Predicate(elem) returns true.
    template <class F>
    void retain(F Predicate)
    {
        Inner.erase(std::remove_if(Inner.begin(), Inner.end(),
                                   [&](T& V) { return !Predicate(V); }),
                    Inner.end());
    }

    /// @brief Whether any element compares equal to X.
    bool contains(const T& X) const
    {
        return std::find(Inner.begin(), Inner.end(), X) != Inner.end();
    }

    /// @brief Index of the first element equal to X, or nullopt.
    std::optional<SizeType> position(const T& X) const
    {
        auto It = std::find(Inner.begin(), Inner.end(), X);
        if (It == Inner.end()) return std::nullopt;
        return static_cast<SizeType>(It - Inner.begin());
    }

    /// @brief Stable sort ascending.
    void sort() { std::stable_sort(Inner.begin(), Inner.end()); }

    /// @brief Unstable sort ascending. Faster than sort(); ordering of equal elements is unspecified.
    void sortUnstable() { std::sort(Inner.begin(), Inner.end()); }

    /// @brief Stable sort with a user-provided comparator.
    template <class F>
    void sortBy(F Cmp) { std::stable_sort(Inner.begin(), Inner.end(), Cmp); }

    /// @brief Remove consecutive runs of equal elements (like std::unique).
    void dedup() { Inner.erase(std::unique(Inner.begin(), Inner.end()), Inner.end()); }

    /// @brief Element-wise equality.
    friend bool operator==(const Vec& L, const Vec& R) { return L.Inner == R.Inner; }

    /// @brief Lexicographic comparison.
    friend auto operator<=>(const Vec& L, const Vec& R) { return L.Inner <=> R.Inner; }

    /// @brief Const reference to the underlying allocator.
    const A& allocator() const noexcept { return Inner.get_allocator().Alloc; }
};

} // namespace goleta
