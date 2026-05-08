#pragma once

/**
 * sysconfig.h — System Configuration
 *
 * リンカスクリプトのシンボル、メモリマップ、ブートモード等の
 * システムレベル設定を一元管理する。
 *
 * C/C++ 両対応:
 *   - C++: constexpr / enum class で型安全な定数を提供
 *   - C  : enum / static inline で同等の機能を提供
 *
 * リンカシンボルは static inline 関数でラップし、
 * extern 宣言が各ソースに散在するのを防ぐ。
 */

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ================================================================== */
/*  Boot Mode (Settings page offset 0)                                */
/* ================================================================== */

typedef enum {
    SYSCONFIG_BOOT_APP = 0,     /**< メインアプリで起動 */
    SYSCONFIG_BOOT_UPDATER = 1, /**< updater モードで起動 */
} sysconfig_boot_mode_t;

/* ================================================================== */
/*  Linker symbols (link-time 解決 — constexpr 不可)                  */
/*                                                                     */
/*  配列型で宣言することで、シンボル自体がアドレスになる。             */
/*  &sym ではなく sym で直接アドレスが取れる。                         */
/* ================================================================== */

extern const uint32_t __settings_start[];
extern const uint32_t __updater_start[];
extern const uint32_t __app_slot_start[];
extern const uint32_t __app_slot_end[];

/** Settings page アドレス (0x7F000) */
static inline uint32_t sysconfig_settings_addr(void) {
    return (uint32_t)__settings_start;
}

/** Settings page への volatile ポインタ (初期化前コード用) */
static inline volatile uint32_t *sysconfig_settings_ptr(void) {
    return (volatile uint32_t *)__settings_start;
}

/** Updater スロット先頭アドレス (0x6E000) */
static inline uint32_t sysconfig_updater_addr(void) {
    return (uint32_t)__updater_start;
}

/** Updater ベクタテーブルへの volatile ポインタ */
static inline volatile uint32_t *sysconfig_updater_vt(void) {
    return (volatile uint32_t *)__updater_start;
}

/** App スロット先頭アドレス (0x26000) */
static inline uint32_t sysconfig_app_slot_addr(void) {
    return (uint32_t)__app_slot_start;
}

/** App スロットサイズ (bytes) */
static inline uint32_t sysconfig_app_slot_size(void) {
    return (uint32_t)(__app_slot_end - __app_slot_start);
}

#ifdef __cplusplus
} /* extern "C" */

/* ================================================================== */
/*  C++ constexpr constants                                           */
/* ================================================================== */

namespace sysconfig {

/* ---- Hardware ---- */
constexpr uint32_t flash_page_size = 4096;

} /* namespace sysconfig */

#else /* C */

/* ================================================================== */
/*  C compile-time constants (enum で型安全に)                        */
/* ================================================================== */

enum {
    SYSCONFIG_FLASH_PAGE_SIZE = 4096,
};

#endif /* __cplusplus */
