/**
 * @file flash_linux.cpp
 * @brief Linux用Flashドライバ（RAMエミュレーション）
 */

#include "flash.h"
#include <sysconfig/flash_layout.h>

#include <cstring>

namespace drivers {
namespace {

constexpr size_t VIRTUAL_FLASH_SIZE = 64 * 1024;
constexpr uint32_t VIRTUAL_FLASH_BASE = sysconfig::flash_layout::LOG_RING_BASE_ADDRESS;
constexpr uint32_t PAGE_SIZE = 4096;

uint8_t s_virtual_flash[VIRTUAL_FLASH_SIZE];
bool s_initialized = false;

void ensure_initialized() {
    if (!s_initialized) {
        std::memset(s_virtual_flash, 0xFF, sizeof(s_virtual_flash));
        s_initialized = true;
    }
}

uint8_t *address_to_ptr(uint32_t address) {
    const uint32_t offset = address - VIRTUAL_FLASH_BASE;
    if (offset >= VIRTUAL_FLASH_SIZE) {
        return nullptr;
    }
    return &s_virtual_flash[offset];
}

}  // namespace

void Flash::page_erase(uint32_t address) {
    ensure_initialized();
    if (auto *ptr = address_to_ptr(address); ptr != nullptr) {
        std::memset(ptr, 0xFF, PAGE_SIZE);
    }
}

void Flash::write(uint32_t address, const void *data, size_t size) {
    ensure_initialized();
    if (auto *ptr = address_to_ptr(address); ptr != nullptr) {
        const auto *src = static_cast<const uint8_t *>(data);
        for (size_t i = 0; i < size; ++i) {
            ptr[i] &= src[i];
        }
    }
}

void Flash::read(uint32_t address, void *data, size_t size) {
    ensure_initialized();
    const auto *ptr = address_to_ptr(address);
    if (ptr != nullptr) {
        std::memcpy(data, ptr, size);
    } else {
        std::memset(data, 0xFF, size);
    }
}

extern "C" void flash_test_reset() {
    std::memset(s_virtual_flash, 0xFF, sizeof(s_virtual_flash));
    s_initialized = true;
}

}  // namespace drivers
