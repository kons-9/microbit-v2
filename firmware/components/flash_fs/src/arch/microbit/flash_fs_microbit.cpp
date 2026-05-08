/**
 * @file flash_fs_microbit.cpp
 * @brief Flash FS nRF52833 バックエンド
 */

#include "arch/flash_fs_arch.h"

#include <nrfx_nvmc.h>
#include <cstring>

void flash_fs_arch_page_erase(uint32_t address) {
    nrfx_nvmc_page_erase(address);
}

void flash_fs_arch_write(uint32_t address, const void *data, size_t size) {
    // nrfx_nvmc_bytes_write はワード境界をまたぐ書き込みに対応
    nrfx_nvmc_bytes_write(address, data, size);
}

void flash_fs_arch_read(uint32_t address, void *buf, size_t size) {
    // NOR Flash はメモリマップされているので直接読める
    std::memcpy(buf, reinterpret_cast<const void *>(address), size);
}
