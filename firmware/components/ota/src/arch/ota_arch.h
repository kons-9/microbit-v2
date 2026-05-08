#pragma once

/**
 * OTA Architecture-specific Interface (internal)
 */

#include <stdint.h>
#include <stddef.h>
#include "ota.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * アプリスロットの指定オフセットを消去
 * @param offset  スロット先頭からのオフセット
 * @param size    消去サイズ (ページ境界にアラインされること)
 */
void ota_arch_flash_erase(uint32_t offset, uint32_t size);

/**
 * アプリスロットへの書き込み
 * @param offset  スロット先頭からのオフセット
 * @param data    書き込みデータ
 * @param len     バイト数 (4バイトアラインされること)
 */
void ota_arch_flash_write(uint32_t offset, const void *data, uint32_t len);

/**
 * アプリスロットからの読み出し (検証用)
 * @param offset  スロット先頭からのオフセット
 * @param buf     読み出し先バッファ
 * @param len     バイト数
 */
void ota_arch_flash_read(uint32_t offset, void *buf, uint32_t len);

/**
 * アプリスロットのサイズを取得
 */
uint32_t ota_arch_get_app_slot_size(void);

/**
 * 指定モードのブートフラグを設定し再起動
 */
void ota_arch_reboot(ota_boot_mode_t mode);

#ifdef __cplusplus
}
#endif
