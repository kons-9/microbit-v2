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

extern "C" {
extern const uint32_t __settings_start[];
extern const uint32_t __recovery_start[];
extern const uint32_t __app_slot_start[];
extern const uint32_t __app_slot_end[];
}

namespace sysconfig {

/* ================================================================== */
/*  Boot Mode (Settings page offset 0)                                */
/* ================================================================== */

enum class BootMode : uint32_t {
    App = 0,      /**< メインアプリで起動 */
    Recovery = 1, /**< リカバリーモードで起動 */
};

constexpr auto BOOT_APP = BootMode::App;
constexpr auto BOOT_RECOVERY = BootMode::Recovery;

/**
 * 次回の起動先を Settings page に保存してリセットする。
 * この関数は成功時には戻らない。
 */
[[noreturn]] void reboot(BootMode mode);

/* ================================================================== */
/*  Linker symbols (link-time 解決 — constexpr 不可)                  */
/*                                                                     */
/*  配列型で宣言することで、シンボル自体がアドレスになる。             */
/*  &sym ではなく sym で直接アドレスが取れる。                         */
/* ================================================================== */

/**
 * Settings page アドレスを取得する (リンカスクリプトで定義)
 * @return Settings page の物理アドレス
 */
inline uint32_t get_settings_address() {
    return static_cast<uint32_t>(reinterpret_cast<uintptr_t>(__settings_start));
}

/**
 * Settings page への volatile ポインタを取得する (初期化前コード用)
 * @return Settings page への volatile ポインタ
 * @post 返却値は有効な Flash アドレスを指す
 */
inline volatile uint32_t *get_settings_pointer() {
    return reinterpret_cast<volatile uint32_t *>(const_cast<uint32_t *>(__settings_start));
}

/**
 * Recovery スロット先頭アドレスを取得する (リンカスクリプトで定義)
 * @return Recovery スロットの物理アドレス
 */
inline uint32_t get_recovery_address() {
    return static_cast<uint32_t>(reinterpret_cast<uintptr_t>(__recovery_start));
}

/**
 * Recovery ベクタテーブルへの volatile ポインタを取得する
 * @return ベクタテーブルへの volatile ポインタ
 */
inline volatile uint32_t *get_recovery_vector_table() {
    return reinterpret_cast<volatile uint32_t *>(const_cast<uint32_t *>(__recovery_start));
}

/**
 * App スロット先頭アドレスを取得する (0x00000)
 * @return App スロットの物理アドレス
 */
inline uint32_t get_app_slot_address() {
    return static_cast<uint32_t>(reinterpret_cast<uintptr_t>(__app_slot_start));
}

/**
 * App スロットサイズを取得する
 * @return App スロットのサイズ (bytes)
 */
inline uint32_t get_app_slot_size() {
    return static_cast<uint32_t>(__app_slot_end - __app_slot_start);
}

/* ================================================================== */
/*  Constants                                                         */
/* ================================================================== */

constexpr uint32_t FLASH_PAGE_SIZE = 4096;

}  // namespace sysconfig
