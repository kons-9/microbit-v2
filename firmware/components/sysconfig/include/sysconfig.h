#pragma once

/**
 * @file sysconfig.h
 * @brief System Configuration — メモリマップ・ブートモード一元管理
 *
 * リンカスクリプトのシンボル、メモリマップ、ブートモード等の
 * システムレベル設定を一元管理する。
 *
 * リンカシンボルは inline 関数でラップし、
 * extern 宣言が各ソースに散在するのを防ぐ。
 *
 * @pre リンカスクリプトで __settings_start 等のシンボルが定義済みであること
 */

#include <cstdint>

/* ================================================================== */
/*  Boot Mode (Settings page offset 0)                                */
/* ================================================================== */

enum class BootMode : uint32_t {
    App = 0,     /**< メインアプリで起動 */
    Updater = 1, /**< updater モードで起動 */
};

using sysconfig_boot_mode_t = BootMode;

constexpr auto SYSCONFIG_BOOT_APP = BootMode::App;
constexpr auto SYSCONFIG_BOOT_UPDATER = BootMode::Updater;

/* ================================================================== */
/*  Linker symbols (link-time 解決 — constexpr 不可)                  */
/*                                                                     */
/*  配列型で宣言することで、シンボル自体がアドレスになる。             */
/*  &sym ではなく sym で直接アドレスが取れる。                         */
/* ================================================================== */

extern "C" {
extern const uint32_t __settings_start[];
extern const uint32_t __updater_start[];
extern const uint32_t __app_slot_start[];
extern const uint32_t __app_slot_end[];
}

/**
 * Settings page アドレスを取得する (0x7F000)
 * @return Settings page の物理アドレス
 */
inline uint32_t sysconfig_get_settings_address() {
    return reinterpret_cast<uint32_t>(__settings_start);
}

/**
 * Settings page への volatile ポインタを取得する (初期化前コード用)
 * @return Settings page への volatile ポインタ
 * @post 返却値は有効な Flash アドレスを指す
 */
inline volatile uint32_t *sysconfig_get_settings_pointer() {
    return reinterpret_cast<volatile uint32_t *>(const_cast<uint32_t *>(__settings_start));
}

/**
 * Updater スロット先頭アドレスを取得する (0x6E000)
 * @return Updater スロットの物理アドレス
 */
inline uint32_t sysconfig_get_updater_address() {
    return reinterpret_cast<uint32_t>(__updater_start);
}

/**
 * Updater ベクタテーブルへの volatile ポインタを取得する
 * @return ベクタテーブルへの volatile ポインタ
 */
inline volatile uint32_t *sysconfig_get_updater_vector_table() {
    return reinterpret_cast<volatile uint32_t *>(const_cast<uint32_t *>(__updater_start));
}

/**
 * App スロット先頭アドレスを取得する (0x00000)
 * @return App スロットの物理アドレス
 */
inline uint32_t sysconfig_get_app_slot_address() {
    return reinterpret_cast<uint32_t>(__app_slot_start);
}

/**
 * App スロットサイズを取得する
 * @return App スロットのサイズ (bytes)
 */
inline uint32_t sysconfig_get_app_slot_size() {
    return static_cast<uint32_t>(__app_slot_end - __app_slot_start);
}

/* ================================================================== */
/*  Constants                                                         */
/* ================================================================== */

namespace sysconfig {

constexpr uint32_t FLASH_PAGE_SIZE = 4096;

} /* namespace sysconfig */
