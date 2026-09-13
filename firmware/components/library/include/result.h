#pragma once

#include <cassert>
#include <utility>
#include <variant>

namespace library {

/** A successful value or an error value. */
template <typename T, typename E>
class Result {
  private:
    struct Error {
        E value;
    };

  public:
    static constexpr Result ok(T value) {
        return Result(std::move(value));
    }

    static constexpr Result err(E error) {
        return Result(Error{std::move(error)});
    }

    constexpr bool has_value() const noexcept {
        return std::holds_alternative<T>(m_data);
    }

    constexpr bool has_error() const noexcept {
        return !has_value();
    }

    constexpr explicit operator bool() const noexcept {
        return has_value();
    }

    constexpr T &value() & {
        assert(has_value());
        return std::get<T>(m_data);
    }

    constexpr const T &value() const & {
        assert(has_value());
        return std::get<T>(m_data);
    }

    constexpr T &&value() && {
        assert(has_value());
        return std::move(std::get<T>(m_data));
    }

    constexpr E &error() & {
        assert(has_error());
        return std::get<Error>(m_data).value;
    }

    constexpr const E &error() const & {
        assert(has_error());
        return std::get<Error>(m_data).value;
    }

    template <typename U>
    constexpr T value_or(U &&fallback) const & {
        return has_value() ? std::get<T>(m_data) : static_cast<T>(std::forward<U>(fallback));
    }

  private:
    constexpr explicit Result(T value)
        : m_data(std::move(value)) {
    }
    constexpr explicit Result(Error error)
        : m_data(std::move(error)) {
    }

    std::variant<T, Error> m_data;
};

template <typename E>
class Result<void, E> {
  private:
    struct Error {
        E value;
    };

  public:
    static constexpr Result ok() {
        return Result();
    }

    static constexpr Result err(E error) {
        return Result(Error{std::move(error)});
    }

    constexpr bool has_value() const noexcept {
        return std::holds_alternative<std::monostate>(m_data);
    }

    constexpr bool has_error() const noexcept {
        return !has_value();
    }

    constexpr explicit operator bool() const noexcept {
        return has_value();
    }

    constexpr void value() const {
        assert(has_value());
    }

    constexpr E &error() & {
        assert(has_error());
        return std::get<Error>(m_data).value;
    }

    constexpr const E &error() const & {
        assert(has_error());
        return std::get<Error>(m_data).value;
    }

  private:
    constexpr Result()
        : m_data(std::monostate{}) {
    }
    constexpr explicit Result(Error error)
        : m_data(std::move(error)) {
    }

    std::variant<std::monostate, Error> m_data;
};

}  // namespace library
