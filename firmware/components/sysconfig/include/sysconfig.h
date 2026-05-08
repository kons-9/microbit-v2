#pragma once

/**
 * @file sysconfig.h
 * @brief System Configuration — メモリマップ・ブートモード一元管理
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
 *
 * @pre リンカスクリプトで __settings_start 等のシンボルが定義済みであること
 */

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ================================================================== */
/*  Boot Mode (Settings page offset 0)                                */
/* ================================================================== */

#ifdef __cplusplus

enum class BootMode : uint32_t {
    App = 0,     /**< メインアプリで起動 */
    Updater = 1, /**< updater モードで起動 */
};

/* NOTE: C 互換のために typedef も用意する */
using sysconfig_boot_mode_t = BootMode;

/* C コードとの互換マクロ */
constexpr auto SYSCONFIG_BOOT_APP = BootMode::App;
constexpr auto SYSCONFIG_BOOT_UPDATER = BootMode::Updater;

#else /* C */

typedef enum {
    SYSCONFIG_BOOT_APP = 0,     /**< メインアプリで起動 */
    SYSCONFIG_BOOT_UPDATER = 1, /**< updater モードで起動 */
} sysconfig_boot_mode_t;

#endif /* __cplusplus */

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

/**
 * Settings page アドレスを取得する (0x7F000)
 * @return Settings page の物理アドレス
 */
static inline uint32_t sysconfig_get_settings_address(void) {
    return (uint32_t)__settings_start;
}

/**
 * Settings page への volatile ポインタを取得する (初期化前コード用)
 * @return Settings page への volatile ポインタ
 * @post 返却値は有効な Flash アドレスを指す
 */
static inline volatile uint32_t *sysconfig_get_settings_pointer(void) {
    return (volatile uint32_t *)__settings_start;
}

/**
 * Updater スロット先頭アドレスを取得する (0x6E000)
 * @return Updater スロットの物理アドレス
 */
static inline uint32_t sysconfig_get_updater_address(void) {
    return (uint32_t)__updater_start;
}

/**
 * Updater ベクタテーブルへの volatile ポインタを取得する
 * @return ベクタテーブルへの volatile ポインタ
 */
static inline volatile uint32_t *sysconfig_get_updater_vector_table(void) {
    return (volatile uint32_t *)__updater_start;
}

/**
 * App スロット先頭アドレスを取得する (0x26000)
 * @return App スロットの物理アドレス
 */
static inline uint32_t sysconfig_get_app_slot_address(void) {
    return (uint32_t)__app_slot_start;
}

/**
 * App スロットサイズを取得する
 * @return App スロットのサイズ (bytes)
 */
static inline uint32_t sysconfig_get_app_slot_size(void) {
    return (uint32_t)(__app_slot_end - __app_slot_start);
}

#ifdef __cplusplus
} /* extern "C" */

/* ================================================================== */
/*  C++ constexpr constants                                           */
/* ================================================================== */

namespace sysconfig {

constexpr uint32_t FLASH_PAGE_SIZE = 4096;

} /* namespace sysconfig */

#else /* C */

/* ================================================================== */
/*  C compile-time constants                                          */
/* ================================================================== */

enum {
    SYSCONFIG_FLASH_PAGE_SIZE = 4096,
};

#endif /* __cplusplus */
