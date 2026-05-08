/**
 * @file ota_microbit.c
 * @brief micro:bit v2 (nRF52833) OTA Flash 操作
 *
 * NOTE: 低レベル Flash 操作のため C で記述する。
 * メモリレイアウトは sysconfig 経由で参照する。
 */

#include "arch/ota_arch.h"

#include <nrf.h>
#include <nrfx_nvmc.h>

#include <sysconfig.h>

void ota_arch_flash_erase(uint32_t offset, uint32_t size) {
    uint32_t address = sysconfig_get_app_slot_address() + offset;
    uint32_t end = address + size;

    /* ページ境界にアライン */
    address &= ~(SYSCONFIG_FLASH_PAGE_SIZE - 1);

    while (address < end) {
        nrfx_nvmc_page_erase(address);
        address += SYSCONFIG_FLASH_PAGE_SIZE;
    }
}

void ota_arch_flash_write(uint32_t offset, const void *data, uint32_t length) {
    uint32_t address = sysconfig_get_app_slot_address() + offset;
    nrfx_nvmc_words_write(address, data, length / 4);

    /* 端数バイト処理 (4バイト未満) */
    uint32_t remainder = length & 3;
    if (remainder > 0) {
        uint32_t word = 0xFFFFFFFF;
        const uint8_t *source = (const uint8_t *)data + (length - remainder);
        for (uint32_t i = 0; i < remainder; i++) {
            word &= ~((uint32_t)0xFF << (i * 8));
            word |= (uint32_t)source[i] << (i * 8);
        }
        nrfx_nvmc_word_write(address + length - remainder, word);
    }
}

void ota_arch_flash_read(uint32_t offset, void *buffer, uint32_t length) {
    uint32_t address = sysconfig_get_app_slot_address() + offset;
    const uint8_t *source = (const uint8_t *)address;
    uint8_t *destination = (uint8_t *)buffer;

    for (uint32_t i = 0; i < length; i++) {
        destination[i] = source[i];
    }
}

uint32_t ota_arch_get_app_slot_size(void) {
    return sysconfig_get_app_slot_size();
}

void ota_arch_reboot(sysconfig_boot_mode_t mode) {
    uint32_t address = sysconfig_get_settings_address();
    nrfx_nvmc_page_erase(address);
    nrfx_nvmc_word_write(address, (uint32_t)mode);
    NVIC_SystemReset();
}
