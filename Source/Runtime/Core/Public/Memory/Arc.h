#pragma once

/// @file
/// @brief Arc<T, A> / Weak<T, A>: atomically reference-counted shared ownership.
/// @note  Implementation currently proxies std::shared_ptr / std::weak_ptr, which use atomic
///        ref counts -- matching Rust's Arc. No non-atomic Rc<T> equivalent is provided;
///        aliasing to std::shared_ptr would silently lie about perf.
///        Unlike Rust's Arc<T>, a goleta::Arc<T> may be null (default construction yields null).
///        The allocator parameter is currently a phantom for type identity: Arc<T, A> and
///        Arc<T, B> are distinct types, but the allocator itself is type-erased into the
///        shared_ptr control block. Swapping to a hand-rolled control block later becomes a
///        non-breaking change.

#include "Memory/Allocator.h"

#include <cassert>
#include <cstddef>
#include <memory>
#include <type_traits>
#include <utility>

namespace goleta {

template <class T, class A = allocators::Global> class Arc;

/// @brief Non-owning reference to a shared T. Upgrade to Arc<T, A> before use.
template <class T, class A = allocators::Global>
class Weak
{
private:
    std::weak_ptr<T> Inner;

    explicit Weak(std::weak_ptr<T> P) noexcept : Inner(std::move(P)) {}
    template <class U, class B> friend class Weak;
    template <class U, class B> friend class Arc;

public:
    using ElementType   = T;
    using AllocatorType = A;

    /// @brief Default-construct an empty Weak that never upgrades.
    Weak() noexcept = default;

    Weak(const Weak&)                = default;
    Weak(Weak&&) noexcept            = default;
    Weak& operator=(const Weak&)     = default;
    Weak& operator=(Weak&&) noexcept = default;
    ~Weak()                          = default;

    /// @brief Attempt to recover an Arc. Returns null Arc if the pointee has been destroyed.
    Arc<T, A> upgrade() const noexcept;

    /// @brief Current strong reference count, or 0 if the pointee has been destroyed.
    long strongCount() const noexcept { return Inner.use_count(); }

    /// @brief Whether the pointee has been destroyed (strong count reached 0).
    bool isExpired() const noexcept { return Inner.expired(); }

    /// @brief Drop this reference; becomes equivalent to a default-constructed Weak.
    void reset() noexcept { Inner.reset(); }

    /// @brief Exchange with Other.
    void swap(Weak& Other) noexcept { Inner.swap(Other.Inner); }
};

/// @brief Atomically reference-counted shared pointer.
/// @tparam T Pointee type.
/// @tparam A Allocator type. Defaults to goleta::allocators::Global.
template <class T, class A>
class Arc
{
private:
    std::shared_ptr<T> Inner;

    explicit Arc(std::shared_ptr<T> P) noexcept : Inner(std::move(P)) {}
    template <class U, class B> friend class Arc;
    template <class U, class B> friend class Weak;

    template <class U, class... Args>
    friend Arc<U> makeArc(Args&&...);
    template <class U, class B, class... Args>
    friend Arc<U, B> makeArcIn(B, Args&&...);

public:
    using ElementType   = T;
    using AllocatorType = A;

    /// @brief Default-construct a null Arc.
    Arc() noexcept = default;

    /// @brief Construct a null Arc from nullptr.
    Arc(std::nullptr_t) noexcept {}

    /// @brief Take ownership of a raw pointer; allocates a control block with the default allocator.
    /// @note  For interop with code that hands out raw owning pointers. Destruction uses `delete P`,
    ///        not A::deallocate -- Raw must be a pointer that `delete` is valid to call on. Prefer
    ///        makeArc / makeArcIn.
    explicit Arc(T* Raw) : Inner(Raw) {}

    Arc(const Arc&) noexcept            = default;
    Arc(Arc&&) noexcept                 = default;
    Arc& operator=(const Arc&) noexcept = default;
    Arc& operator=(Arc&&) noexcept      = default;
    ~Arc()                              = default;

    /// @brief Construct from a compatible Arc<U, A> where U* converts to T*.
    template <class U>
        requires (!std::is_same_v<U, T>) && std::is_convertible_v<U*, T*>
    Arc(const Arc<U, A>& Other) noexcept : Inner(Other.Inner) {}

    template <class U>
        requires (!std::is_same_v<U, T>) && std::is_convertible_v<U*, T*>
    Arc(Arc<U, A>&& Other) noexcept : Inner(std::move(Other.Inner)) {}

    /// @brief Dereference; asserts if null.
    T& operator*() const { assert(Inner); return *Inner; }

    /// @brief Member access; asserts if null.
    T* operator->() const noexcept { assert(Inner); return Inner.get(); }

    /// @brief Raw pointer. Null if the Arc is null.
    T* get() const noexcept { return Inner.get(); }

    /// @brief Whether this Arc holds a value.
    explicit operator bool() const noexcept { return static_cast<bool>(Inner); }

    /// @brief Whether the Arc is null.
    bool isNull() const noexcept { return !Inner; }

    /// @brief Current strong reference count, including this one.
    long strongCount() const noexcept { return Inner.use_count(); }

    /// @brief Produce a non-owning Weak reference to the same pointee.
    Weak<T, A> downgrade() const noexcept { return Weak<T, A>{std::weak_ptr<T>(Inner)}; }

    /// @brief Drop this reference; becomes null.
    void reset() noexcept { Inner.reset(); }

    /// @brief Exchange with Other.
    void swap(Arc& Other) noexcept { Inner.swap(Other.Inner); }

    /// @brief Non-owning access to the underlying std::shared_ptr. Escape hatch for interop.
    const std::shared_ptr<T>& asSharedPtr() const noexcept { return Inner; }
};

template <class T, class A>
Arc<T, A> Weak<T, A>::upgrade() const noexcept
{
    return Arc<T, A>{Inner.lock()};
}

/// @brief Allocate a T on the heap and return a non-null Arc sharing ownership.
template <class T, class... Args>
Arc<T> makeArc(Args&&... ArgsIn)
{
    return Arc<T>{std::make_shared<T>(std::forward<Args>(ArgsIn)...)};
}

/// @brief Allocate a T through Alloc (control block + value) and return a non-null Arc.
template <class T, class A, class... Args>
Arc<T, A> makeArcIn(A Alloc, Args&&... ArgsIn)
{
    return Arc<T, A>{std::allocate_shared<T>(
        detail::StdAllocatorAdapter<T, A>{std::move(Alloc)},
        std::forward<Args>(ArgsIn)...)};
}

template <class T, class A>
bool operator==(const Arc<T, A>& P, std::nullptr_t) noexcept { return P.isNull(); }

template <class T, class A>
bool operator==(std::nullptr_t, const Arc<T, A>& P) noexcept { return P.isNull(); }

/// @brief Identity comparison: true iff both Arcs point at the same object. Mirrors Rust Arc::ptr_eq.
/// @note  This is not a value comparison. Two Arcs constructed independently with equal T values
///        compare unequal.
template <class T, class A>
bool operator==(const Arc<T, A>& L, const Arc<T, A>& R) noexcept { return L.get() == R.get(); }

} // namespace goleta
