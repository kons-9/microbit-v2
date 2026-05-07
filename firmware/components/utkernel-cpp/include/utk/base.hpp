#pragma once

#include <tk/tkernel.h>

namespace utk {

// Error code type alias
using err_t = ER;

// Check if error code indicates success
inline bool is_ok(err_t e) { return e >= 0; }

// Non-copyable base
class non_copyable {
protected:
    non_copyable() = default;
    ~non_copyable() = default;
    non_copyable(const non_copyable&) = delete;
    non_copyable& operator=(const non_copyable&) = delete;
    non_copyable(non_copyable&&) = default;
    non_copyable& operator=(non_copyable&&) = default;
};

} // namespace utk
