#pragma once

/// @file
/// @brief Rc<T> / RefCounted: intrusive atomically ref-counted smart pointer.
/// @note  Naming caveat: Rust's std::rc::Rc is single-threaded and non-atomic; this Rc is
///        atomic, functionally matching Rust's std::sync::Arc. The name is chosen for
///        ergonomics inside the engine -- do not conflate with Rust's non-atomic Rc.

#include <atomic>
#include <cassert>
#include <cstddef>
#include <type_traits>
#include <utility>

namespace goleta
{

/// @brief Mixin base for types managed by Rc<T>. Holds an atomic reference count.
/// @note  Objects deriving from RefCounted must be heap-allocated (prefer makeRc<T>).
///        Stack-allocating a RefCounted subclass and wrapping it in an Rc leads to a
///        double-free on scope exit: the Rc calls release() which runs `delete this` on
///        stack memory. The destructor is protected to forbid `delete basePtr` from outside,
///        but cannot prevent `delete derivedPtr`; treat the convention as documented.
///        Header-only: no CORE_API export — all members are inline, and the vtable is
///        emitted as COMDAT in each TU that instantiates a derived class.
class RefCounted
{
public:
    /// @brief Atomically increment the reference count.
    void addRef() const noexcept { RefCount.fetch_add(1, std::memory_order_relaxed); }

    /// @brief Atomically decrement the reference count; delete this when it reaches 0.
    void release() const noexcept
    {
        if (RefCount.fetch_sub(1, std::memory_order_acq_rel) == 1)
        {
            delete this;
        }
    }

    /// @brief Current reference count. Informational; do not gate correctness on it.
    long refCount() const noexcept { return RefCount.load(std::memory_order_acquire); }

    RefCounted(const RefCounted&) = delete;
    RefCounted& operator=(const RefCounted&) = delete;

protected:
    RefCounted() noexcept = default;
    virtual ~RefCounted() = default;

private:
    mutable std::atomic<long> RefCount{0};
};

/// @brief Intrusive atomically ref-counted smart pointer.
/// @tparam T Pointee type; must provide addRef() / release() (typically by deriving from RefCounted).
/// @note  May be null. Use makeRc<T>(args...) for a guaranteed non-null instance.
///        Rc(T*) uses aliasing semantics: it calls addRef() on the raw pointer. For a
///        freshly new'd RefCounted with refCount == 0 this produces refCount == 1.
template <class T>
class Rc
{
public:
    using ElementType = T;

    /// @brief Null Rc.
    Rc() noexcept = default;

    /// @brief Null Rc from nullptr.
    Rc(std::nullptr_t) noexcept {}

    /// @brief Wrap Raw and bump its reference count. Pass nullptr for a null Rc.
    explicit Rc(T* Raw) noexcept
        : Ptr(Raw)
    {
        if (Ptr)
            Ptr->addRef();
    }

    Rc(const Rc& Other) noexcept
        : Ptr(Other.Ptr)
    {
        if (Ptr)
            Ptr->addRef();
    }

    Rc(Rc&& Other) noexcept
        : Ptr(Other.Ptr)
    {
        Other.Ptr = nullptr;
    }

    /// @brief Up-cast copy from a compatible Rc<U>.
    template <class U>
        requires(!std::is_same_v<U, T>) && std::is_convertible_v<U*, T*>
    Rc(const Rc<U>& Other) noexcept
        : Ptr(Other.get())
    {
        if (Ptr)
            Ptr->addRef();
    }

    /// @brief Up-cast move from a compatible Rc<U>.
    template <class U>
        requires(!std::is_same_v<U, T>) && std::is_convertible_v<U*, T*>
    Rc(Rc<U>&& Other) noexcept
        : Ptr(Other.leak())
    {
    }

    ~Rc()
    {
        if (Ptr)
            Ptr->release();
    }

    Rc& operator=(const Rc& Other) noexcept
    {
        if (this != &Other)
        {
            T* OldPtr = Ptr;
            Ptr = Other.Ptr;
            if (Ptr)
                Ptr->addRef();
            if (OldPtr)
                OldPtr->release();
        }
        return *this;
    }

    Rc& operator=(Rc&& Other) noexcept
    {
        if (this != &Other)
        {
            T* OldPtr = Ptr;
            Ptr = Other.Ptr;
            Other.Ptr = nullptr;
            if (OldPtr)
                OldPtr->release();
        }
        return *this;
    }

    /// @brief Dereference; asserts if null.
    T& operator*() const noexcept
    {
        assert(Ptr);
        return *Ptr;
    }

    /// @brief Member access; asserts if null.
    T* operator->() const noexcept
    {
        assert(Ptr);
        return Ptr;
    }

    /// @brief Raw pointer access; nullptr for a null Rc.
    T* get() const noexcept { return Ptr; }

    /// @brief Whether this Rc holds a non-null pointer.
    explicit operator bool() const noexcept { return Ptr != nullptr; }

    /// @brief Whether this Rc is null.
    bool isNull() const noexcept { return Ptr == nullptr; }

    /// @brief Release ownership without decrementing the refcount; Rc becomes null.
    /// @note  Caller must eventually release() or re-wrap in an Rc.
    T* leak() noexcept
    {
        T* Out = Ptr;
        Ptr = nullptr;
        return Out;
    }

    /// @brief Replace the managed object. Drops the old reference (if any), adds one on Raw.
    void reset(T* Raw = nullptr) noexcept
    {
        T* OldPtr = Ptr;
        Ptr = Raw;
        if (Ptr)
            Ptr->addRef();
        if (OldPtr)
            OldPtr->release();
    }

    /// @brief Exchange contents with Other.
    void swap(Rc& Other) noexcept
    {
        T* Tmp = Ptr;
        Ptr = Other.Ptr;
        Other.Ptr = Tmp;
    }

private:
    T* Ptr = nullptr;
};

/// @brief Heap-allocate a T and return a non-null Rc with ref count 1.
template <class T, class... Args>
Rc<T> makeRc(Args&&... ArgsIn)
{
    return Rc<T>{new T(std::forward<Args>(ArgsIn)...)};
}

template <class T, class U>
bool operator==(const Rc<T>& A, const Rc<U>& B) noexcept
{
    return A.get() == B.get();
}

template <class T>
bool operator==(const Rc<T>& A, std::nullptr_t) noexcept
{
    return A.isNull();
}

template <class T>
bool operator==(std::nullptr_t, const Rc<T>& A) noexcept
{
    return A.isNull();
}

} // namespace goleta
