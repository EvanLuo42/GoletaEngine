#pragma once

/// @file
/// @brief Result<T, E>: Rust-style tagged union for fallible operations. Ok{value} / Err{error}
///        wrap the two variants; value() / error() assert on the wrong variant.

#include <cassert>
#include <optional>
#include <type_traits>
#include <utility>
#include <variant>

namespace goleta
{

/// @brief Ok tag; wrap the success value and let implicit conversion produce a Result.
template <class T>
struct Ok
{
    T Value;
};
template <class T> Ok(T) -> Ok<T>;

/// @brief Err tag; wrap the error value and let implicit conversion produce a Result.
template <class E>
struct Err
{
    E Error;
};
template <class E> Err(E) -> Err<E>;

template <class T, class E>
class Result
{
public:
    using ValueType = T;
    using ErrorType = E;

    template <class U>
        requires std::is_convertible_v<U&&, T>
    Result(Ok<U> V) : Storage_(std::in_place_index<0>, T(std::move(V.Value)))
    {
    }

    template <class F>
        requires std::is_convertible_v<F&&, E>
    Result(Err<F> Er) : Storage_(std::in_place_index<1>, E(std::move(Er.Error)))
    {
    }

    [[nodiscard]] bool isOk() const noexcept { return Storage_.index() == 0; }
    [[nodiscard]] bool isErr() const noexcept { return Storage_.index() == 1; }
    explicit           operator bool() const noexcept { return isOk(); }

    T&       value() & { assert(isOk()); return std::get<0>(Storage_); }
    const T& value() const& { assert(isOk()); return std::get<0>(Storage_); }
    T&&      value() && { assert(isOk()); return std::get<0>(std::move(Storage_)); }

    E&       error() & { assert(isErr()); return std::get<1>(Storage_); }
    const E& error() const& { assert(isErr()); return std::get<1>(Storage_); }
    E&&      error() && { assert(isErr()); return std::get<1>(std::move(Storage_)); }

    template <class U>
    T valueOr(U&& Fallback) const&
    {
        return isOk() ? std::get<0>(Storage_) : T(std::forward<U>(Fallback));
    }

private:
    std::variant<T, E> Storage_;
};

/// @brief Void-success specialization: success carries no payload. Default construction is Ok;
///        return `Err{...}` for the error variant.
template <class E>
class Result<void, E>
{
public:
    using ValueType = void;
    using ErrorType = E;

    Result() = default;

    template <class F>
        requires std::is_convertible_v<F&&, E>
    Result(Err<F> Er) : Error_(E(std::move(Er.Error)))
    {
    }

    [[nodiscard]] bool isOk() const noexcept { return !Error_.has_value(); }
    [[nodiscard]] bool isErr() const noexcept { return Error_.has_value(); }
    explicit           operator bool() const noexcept { return isOk(); }

    E&       error() & { assert(isErr()); return *Error_; }
    const E& error() const& { assert(isErr()); return *Error_; }
    E&&      error() && { assert(isErr()); return std::move(*Error_); }

private:
    std::optional<E> Error_;
};

} // namespace goleta
