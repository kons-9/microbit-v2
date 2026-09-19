#pragma once

#include <cstdint>

namespace sysconfig::flash_layout {

inline constexpr uint32_t LOG_RING_BASE_ADDRESS = 0x70000;
inline constexpr uint32_t LOG_FIXED_BASE_ADDRESS = 0x73000;
inline constexpr uint32_t SETTINGS_BASE_ADDRESS = 0x74000;
inline constexpr uint32_t CALIB_BASE_ADDRESS = 0x75000;

constexpr uint32_t log_ring_base_address() noexcept {
    return LOG_RING_BASE_ADDRESS;
}

constexpr uint32_t log_fixed_base_address() noexcept {
    return LOG_FIXED_BASE_ADDRESS;
}

constexpr uint32_t settings_base_address() noexcept {
    return SETTINGS_BASE_ADDRESS;
}

constexpr uint32_t calib_base_address() noexcept {
    return CALIB_BASE_ADDRESS;
}

}  // namespace sysconfig::flash_layout
