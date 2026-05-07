#include "arch/ota_arch.h"

#include <nrf.h>
#include <nrfx_nvmc.h>

/**
 * micro:bit v2 (nRF52833) OTA Flash操作
 *
 * メモリレイアウトはリンカスクリプト (memory_map.ld) で一元管理。
 * シンボル参照により、レイアウト変更時にここを修正する必要はない。
 */

/* リンカスクリプトで定義されたシンボル */
extern const uint32_t __app_slot_start;
extern const uint32_t __app_slot_end;
extern const uint32_t __settings_start;

#define APP_SLOT_ADDR ((uint32_t) & __app_slot_start)
#define APP_SLOT_SIZE ((uint32_t) & __app_slot_end - (uint32_t) & __app_slot_start)
#define FLASH_PAGE_SIZE 4096

#define SETTINGS_ADDR ((uint32_t) & __settings_start)

void ota_arch_flash_erase(uint32_t offset, uint32_t size) {
    uint32_t addr = APP_SLOT_ADDR + offset;
    uint32_t end = addr + size;

    /* ページ境界にアライン */
    addr &= ~(FLASH_PAGE_SIZE - 1);

    while (addr < end) {
        nrfx_nvmc_page_erase(addr);
        addr += FLASH_PAGE_SIZE;
    }
}

void ota_arch_flash_write(uint32_t offset, const void *data, uint32_t len) {
    uint32_t addr = APP_SLOT_ADDR + offset;
    nrfx_nvmc_words_write(addr, data, len / 4);

    /* 端数バイトがある場合 (4バイト未満) */
    uint32_t remainder = len & 3;
    if (remainder > 0) {
        uint32_t word = 0xFFFFFFFF;
        const uint8_t *src = (const uint8_t *)data + (len - remainder);
        for (uint32_t i = 0; i < remainder; i++) {
            word &= ~(0xFF << (i * 8));
            word |= (uint32_t)src[i] << (i * 8);
        }
        nrfx_nvmc_word_write(addr + len - remainder, word);
    }
}

void ota_arch_flash_read(uint32_t offset, void *buf, uint32_t len) {
    uint32_t addr = APP_SLOT_ADDR + offset;
    const uint8_t *src = (const uint8_t *)addr;
    uint8_t *dst = (uint8_t *)buf;

    for (uint32_t i = 0; i < len; i++) {
        dst[i] = src[i];
    }
}

uint32_t ota_arch_get_app_slot_size(void) {
    return APP_SLOT_SIZE;
}

void ota_arch_reboot(ota_boot_mode_t mode) {
    nrfx_nvmc_page_erase(SETTINGS_ADDR);
    nrfx_nvmc_word_write(SETTINGS_ADDR, (uint32_t)mode);
    NVIC_SystemReset();
}
