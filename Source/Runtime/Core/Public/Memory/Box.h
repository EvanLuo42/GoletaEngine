#pragma once

/// @file
/// @brief Box<T, A>: unique-owning heap pointer, Rust-shaped. Stores an allocator for release.
/// @note  Unlike Rust's Box<T>, a goleta::Box<T> may be null (matching C++ unique_ptr semantics).
///        Default construction yields a null Box; makeBox<T>(args...) always returns a non-null Box.

#include "Memory/Allocator.h"

#include <cassert>
#include <cstddef>
#include <memory>
#include <type_traits>
#include <utility>

namespace goleta {

namespace detail {

/// @brief Deleter used by Box<T, A>: calls T's destructor then returns storage via A.
/// @note  Inherits from A so that stateless allocators cost zero bytes via EBO.
template <class T, class A>
struct BoxDeleter : private A
{
    BoxDeleter() = default;
    explicit BoxDeleter(A InAlloc) noexcept : A(std::move(InAlloc)) {}

    A&       allocator()       noexcept { return static_cast<A&>(*this); }
    const A& allocator() const noexcept { return static_cast<const A&>(*this); }

    void operator()(T* P) const noexcept
    {
        if (!P) return;
        P->~T();
        // allocator() is logically-mutable state; the deleter is invoked as const from unique_ptr.
        const_cast<BoxDeleter*>(this)->allocator().template deallocate<T>(P, 1);
    }
};

} // namespace detail

/// @brief Unique-owning smart pointer.
/// @tparam T Pointee type.
/// @tparam A Allocator type. Defaults to goleta::allocators::Global.
/// @note  A slicing upcast Box<Derived> -> Box<Base> is allowed when Base* is convertible from
///        Derived*. As with std::unique_ptr<T>, if T has no virtual destructor the deleter
///        will invoke Base::~Base and free Base-sized storage, which is undefined behavior when
///        the actual object is a Derived with a different size. Only upcast across virtual
///        hierarchies.
template <class T, class A = allocators::Global>
class Box
{
private:
    using Deleter = detail::BoxDeleter<T, A>;
    std::unique_ptr<T, Deleter> Inner;

    template <class U, class B> friend class Box;

public:
    using ElementType   = T;
    using Pointer       = T*;
    using AllocatorType = A;

    /// @brief Construct a null Box with a default-constructed allocator.
    Box() noexcept = default;

    /// @brief Construct a null Box from nullptr.
    Box(std::nullptr_t) noexcept {}

    /// @brief Take ownership of a raw pointer, using the default-constructed allocator to free it.
    /// @note  Raw must have been obtained from A{}.allocate<T>(1), or behavior on destruction is
    ///        undefined. Prefer makeBox / makeBoxIn.
    explicit Box(T* Raw) noexcept : Inner(Raw) {}

    /// @brief Take ownership of a raw pointer with an explicit allocator.
    Box(T* Raw, A Alloc) noexcept : Inner(Raw, Deleter{std::move(Alloc)}) {}

    Box(Box&&) noexcept = default;

    /// @brief Move-construct from a compatible Box<U, A> where U* converts to T*.
    template <class U>
        requires (!std::is_same_v<U, T>) && std::is_convertible_v<U*, T*>
    Box(Box<U, A>&& Other) noexcept
        : Inner(Other.Inner.release(), Deleter{std::move(Other.Inner.get_deleter().allocator())})
    {
    }

    Box(const Box&)            = delete;
    Box& operator=(const Box&) = delete;

    Box& operator=(Box&&) noexcept = default;

    ~Box() = default;

    /// @brief Dereference; asserts if null.
    T&       operator*()       { assert(Inner); return *Inner; }
    const T& operator*() const { assert(Inner); return *Inner; }

    /// @brief Member access; asserts if null.
    T*       operator->()       noexcept { assert(Inner); return Inner.get(); }
    const T* operator->() const noexcept { assert(Inner); return Inner.get(); }

    /// @brief Raw pointer access. Returns nullptr if the Box is null.
    T*       get()       noexcept { return Inner.get(); }
    const T* get() const noexcept { return Inner.get(); }

    /// @brief Reference to the pointee; asserts if null. Mirrors Rust Box::as_ref.
    const T& asRef() const { assert(Inner); return *Inner; }

    /// @brief Mutable reference to the pointee; asserts if null. Mirrors Rust Box::as_mut.
    T& asMut() { assert(Inner); return *Inner; }

    /// @brief Whether the Box currently owns an object.
    explicit operator bool() const noexcept { return static_cast<bool>(Inner); }

    /// @brief Whether the Box is null.
    bool isNull() const noexcept { return !Inner; }

    /// @brief Release ownership and return the raw pointer. The Box becomes null.
    /// @note  Caller takes over responsibility for destroying the object and returning storage to
    ///        an equivalent allocator. Prefer leak() when the value is meant to outlive the Box.
    T* release() noexcept { return Inner.release(); }

    /// @brief Release ownership and return the raw pointer, intentionally never freed.
    T* leak() noexcept { return Inner.release(); }

    /// @brief Replace the managed object; destroys the previous one (if any).
    void reset(T* Raw = nullptr) noexcept { Inner.reset(Raw); }

    /// @brief Exchange contents with Other.
    void swap(Box& Other) noexcept { Inner.swap(Other.Inner); }

    /// @brief Const access to the stored allocator.
    const A& allocator() const noexcept { return Inner.get_deleter().allocator(); }
};

/// @brief Heap-allocate a T via the default allocator and return a non-null Box owning it.
template <class T, class... Args>
Box<T> makeBox(Args&&... ArgsIn)
{
    allocators::Global Alloc{};
    T* P = Alloc.template allocate<T>(1);
    try
    {
        ::new (static_cast<void*>(P)) T(std::forward<Args>(ArgsIn)...);
    }
    catch (...)
    {
        Alloc.template deallocate<T>(P, 1);
        throw;
    }
    return Box<T>{P, std::move(Alloc)};
}

/// @brief Heap-allocate a T via Alloc and return a non-null Box owning it.
template <class T, class A, class... Args>
Box<T, A> makeBoxIn(A Alloc, Args&&... ArgsIn)
{
    T* P = Alloc.template allocate<T>(1);
    try
    {
        ::new (static_cast<void*>(P)) T(std::forward<Args>(ArgsIn)...);
    }
    catch (...)
    {
        Alloc.template deallocate<T>(P, 1);
        throw;
    }
    return Box<T, A>{P, std::move(Alloc)};
}

template <class T, class A>
bool operator==(const Box<T, A>& B, std::nullptr_t) noexcept { return B.isNull(); }

template <class T, class A>
bool operator==(std::nullptr_t, const Box<T, A>& B) noexcept { return B.isNull(); }

} // namespace goleta
