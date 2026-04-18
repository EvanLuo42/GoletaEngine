#pragma once

/// @file
/// @brief Allocator model: a Rust-style universal allocator type and a std-compatible adapter.

#include <cstddef>
#include <new>
#include <type_traits>
#include <utility>

namespace goleta {

/// @brief Concept-by-convention for "universal" allocators usable with goleta containers.
/// @note  An allocator type A must provide:
///          template <class T> T*   allocate(size_t N) const;
///          template <class T> void deallocate(T* P, size_t N) const noexcept;
///          bool operator==(const A&) const noexcept;
///        A may be stateless (empty ZST) or carry state. Rebinding across value types is
///        handled automatically by StdAllocatorAdapter.
///
///        An allocator may opt in to container-assignment / container-swap propagation and
///        advertise always-equality by publishing any of these nested types:
///          using PropagateOnCopy = std::true_type;   // default: is_empty<A>
///          using PropagateOnMove = std::true_type;   // default: is_empty<A>
///          using PropagateOnSwap = std::true_type;   // default: is_empty<A>
///          using IsAlwaysEqual   = std::true_type;   // default: is_empty<A>
///        Omit a trait to accept the default. Use std::false_type to disable propagation for
///        allocators whose state must remain bound to its original container (arenas, stack
///        allocators, per-frame bump allocators).

namespace allocators {

/// @brief Default global allocator: aligned new/delete. Zero-sized, always-equal.
struct Global
{
    using PropagateOnCopy = std::true_type;
    using PropagateOnMove = std::true_type;
    using PropagateOnSwap = std::true_type;
    using IsAlwaysEqual   = std::true_type;

    Global() = default;

    template <class T>
    [[nodiscard]] T* allocate(std::size_t N) const
    {
        if constexpr (alignof(T) > __STDCPP_DEFAULT_NEW_ALIGNMENT__)
        {
            return static_cast<T*>(
                ::operator new(N * sizeof(T), std::align_val_t{alignof(T)}));
        }
        else
        {
            return static_cast<T*>(::operator new(N * sizeof(T)));
        }
    }

    template <class T>
    void deallocate(T* P, std::size_t N) const noexcept
    {
        if constexpr (alignof(T) > __STDCPP_DEFAULT_NEW_ALIGNMENT__)
        {
            ::operator delete(static_cast<void*>(P),
                              N * sizeof(T),
                              std::align_val_t{alignof(T)});
        }
        else
        {
            ::operator delete(static_cast<void*>(P), N * sizeof(T));
        }
    }

    bool operator==(const Global&) const noexcept { return true; }
    bool operator!=(const Global&) const noexcept { return false; }
};

} // namespace allocators

namespace detail {

template <class A, class = void>
struct AllocPropagateOnCopy : std::bool_constant<std::is_empty_v<A>> {};
template <class A>
struct AllocPropagateOnCopy<A, std::void_t<typename A::PropagateOnCopy>>
    : A::PropagateOnCopy {};

template <class A, class = void>
struct AllocPropagateOnMove : std::bool_constant<std::is_empty_v<A>> {};
template <class A>
struct AllocPropagateOnMove<A, std::void_t<typename A::PropagateOnMove>>
    : A::PropagateOnMove {};

template <class A, class = void>
struct AllocPropagateOnSwap : std::bool_constant<std::is_empty_v<A>> {};
template <class A>
struct AllocPropagateOnSwap<A, std::void_t<typename A::PropagateOnSwap>>
    : A::PropagateOnSwap {};

template <class A, class = void>
struct AllocIsAlwaysEqual : std::bool_constant<std::is_empty_v<A>> {};
template <class A>
struct AllocIsAlwaysEqual<A, std::void_t<typename A::IsAlwaysEqual>>
    : A::IsAlwaysEqual {};

/// @brief Adapter that makes a goleta universal allocator satisfy the std Allocator concept.
/// @note  Rebinding is automatic: allocator_traits picks StdAllocatorAdapter<U, A> via template
///        argument substitution since T is the first template parameter.
/// @note  The snake_case member aliases below are mandated by the C++ standard --
///        std::allocator_traits looks them up by exactly these identifiers. Renaming them to
///        project-style UpperCamelCase would make the adapter stop being a valid Allocator.
template <class T, class A>
struct StdAllocatorAdapter
{
    using value_type = T;

    using propagate_on_container_copy_assignment = AllocPropagateOnCopy<A>;
    using propagate_on_container_move_assignment = AllocPropagateOnMove<A>;
    using propagate_on_container_swap            = AllocPropagateOnSwap<A>;
    using is_always_equal                        = AllocIsAlwaysEqual<A>;

    A Alloc{};

    StdAllocatorAdapter() = default;
    explicit StdAllocatorAdapter(A InAlloc) noexcept : Alloc(std::move(InAlloc)) {}

    template <class U>
    StdAllocatorAdapter(const StdAllocatorAdapter<U, A>& Other) noexcept
        : Alloc(Other.Alloc)
    {
    }

    [[nodiscard]] T* allocate(std::size_t N)
    {
        return Alloc.template allocate<T>(N);
    }

    void deallocate(T* P, std::size_t N) noexcept
    {
        Alloc.template deallocate<T>(P, N);
    }

    bool operator==(const StdAllocatorAdapter& Other) const noexcept
    {
        return Alloc == Other.Alloc;
    }
    bool operator!=(const StdAllocatorAdapter& Other) const noexcept
    {
        return !(*this == Other);
    }
};

} // namespace detail
} // namespace goleta
