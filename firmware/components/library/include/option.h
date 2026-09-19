#pragma once

#include <cassert>
#include <optional>
#include <utility>

namespace library {

/** A value that may or may not be present. */
template <typename T>
class Option {
  public:
    constexpr Option() noexcept = default;

    static constexpr Option some(T value) {
        return Option(std::move(value));
    }

    static constexpr Option none() noexcept {
        return Option();
    }

    constexpr bool has_value() const noexcept {
        return m_value.has_value();
    }

    constexpr explicit operator bool() const noexcept {
        return has_value();
    }

    constexpr T &value() & {
        assert(has_value());
        return *m_value;
    }

    constexpr const T &value() const & {
        assert(has_value());
        return *m_value;
    }

    constexpr T &&value() && {
        assert(has_value());
        return std::move(*m_value);
    }

    constexpr T &operator*() & {
        return value();
    }

    constexpr const T &operator*() const & {
        return value();
    }

    constexpr T *operator->() {
        assert(has_value());
        return &*m_value;
    }

    constexpr const T *operator->() const {
        assert(has_value());
        return &*m_value;
    }

    template <typename U>
    constexpr T value_or(U &&fallback) const & {
        return has_value() ? *m_value : static_cast<T>(std::forward<U>(fallback));
    }

  private:
    constexpr explicit Option(T value)
        : m_value(std::move(value)) {
    }

    std::optional<T> m_value;
};

}  // namespace library
