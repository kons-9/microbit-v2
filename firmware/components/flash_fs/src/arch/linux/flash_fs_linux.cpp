/**
 * @file flash_fs_linux.cpp
 * @brief Flash FS Linux テスト用バックエンド (RAM エミュレーション)
 */

#include "arch/flash_fs_arch.h"

#include <cstring>
#include <cstdint>

/* 64KB の仮想 Flash (RAM上) */
static constexpr size_t VIRTUAL_FLASH_SIZE = 64 * 1024;
static constexpr uint32_t VIRTUAL_FLASH_BASE = 0x70000;
static constexpr uint32_t PAGE_SIZE = 4096;

static uint8_t s_virtualFlash[VIRTUAL_FLASH_SIZE];
static bool s_initialized = false;

static void ensure_initialized() {
    if (!s_initialized) {
        std::memset(s_virtualFlash, 0xFF, sizeof(s_virtualFlash));
        s_initialized = true;
    }
}

/**
 * @brief テスト用: 仮想Flashを全消去してリセットする
 */
extern "C" void flash_fs_arch_test_reset() {
    std::memset(s_virtualFlash, 0xFF, sizeof(s_virtualFlash));
    s_initialized = true;
}

static uint8_t *AddressToPtr(uint32_t address) {
    uint32_t offset = address - VIRTUAL_FLASH_BASE;
    if (offset >= VIRTUAL_FLASH_SIZE) {
        return nullptr;
    }
    return &s_virtualFlash[offset];
}

void flash_fs_arch_page_erase(uint32_t address) {
    ensure_initialized();
    uint8_t *ptr = AddressToPtr(address);
    if (ptr != nullptr) {
        std::memset(ptr, 0xFF, PAGE_SIZE);
    }
}

void flash_fs_arch_write(uint32_t address, const void *data, size_t size) {
    ensure_initialized();
    uint8_t *ptr = AddressToPtr(address);
    if (ptr != nullptr) {
        // NOR Flash のセマンティクス: 1→0 のみ (AND動作)
        const auto *src = static_cast<const uint8_t *>(data);
        for (size_t i = 0; i < size; ++i) {
            ptr[i] &= src[i];
        }
    }
}

void flash_fs_arch_read(uint32_t address, void *buf, size_t size) {
    ensure_initialized();
    const uint8_t *ptr = AddressToPtr(address);
    if (ptr != nullptr) {
        std::memcpy(buf, ptr, size);
    } else {
        std::memset(buf, 0xFF, size);
    }
}
