#pragma once

/**
 * @file ota_arch.h
 * @brief OTA アーキテクチャ固有インターフェース (内部用)
 *
 * 各ターゲットが Flash 操作とリブートを実装する。
 */

#include <stdint.h>
#include <stddef.h>

#include <sysconfig.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * アプリスロットの指定オフセットを消去する
 *
 * @pre  offset がページ境界にアラインされていること
 * @param offset  スロット先頭からのオフセット
 * @param size    消去サイズ (ページ境界にアラインされること)
 */
void ota_arch_flash_erase(uint32_t offset, uint32_t size);

/**
 * アプリスロットへ書き込む
 *
 * @pre  offset が 4 バイトアラインされていること
 * @param offset  スロット先頭からのオフセット
 * @param data    書き込みデータ
 * @param length  バイト数
 */
void ota_arch_flash_write(uint32_t offset, const void *data, uint32_t length);

/**
 * アプリスロットから読み出す (検証用)
 *
 * @param offset  スロット先頭からのオフセット
 * @param buffer  読み出し先バッファ
 * @param length  バイト数
 */
void ota_arch_flash_read(uint32_t offset, void *buffer, uint32_t length);

/**
 * アプリスロットのサイズを取得する
 * @return スロットサイズ (bytes)
 */
uint32_t ota_arch_get_app_slot_size(void);

/**
 * 指定モードのブートフラグを設定し再起動する
 *
 * @post この関数からは返らない
 * @param mode  ブートモード
 */
void ota_arch_reboot(sysconfig_boot_mode_t mode);

#ifdef __cplusplus
}
#endif
