#pragma once

#include <cstdint>

extern "C" {
extern const uint32_t __fs_log_ring_start[];
extern const uint32_t __fs_log_fixed_start[];
extern const uint32_t __fs_settings_start[];
extern const uint32_t __fs_calib_start[];
}

namespace sysconfig::flash_layout {

inline uint32_t log_ring_base_address() noexcept {
    return static_cast<uint32_t>(reinterpret_cast<uintptr_t>(__fs_log_ring_start));
}

inline uint32_t log_fixed_base_address() noexcept {
    return static_cast<uint32_t>(reinterpret_cast<uintptr_t>(__fs_log_fixed_start));
}

inline uint32_t settings_base_address() noexcept {
    return static_cast<uint32_t>(reinterpret_cast<uintptr_t>(__fs_settings_start));
}

inline uint32_t calib_base_address() noexcept {
    return static_cast<uint32_t>(reinterpret_cast<uintptr_t>(__fs_calib_start));
}

}  // namespace sysconfig::flash_layout
