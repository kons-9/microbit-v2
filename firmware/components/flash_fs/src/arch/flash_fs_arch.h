#pragma once

/**
 * @file flash_fs_arch.h
 * @brief Flash FS アーキテクチャ抽象層
 */

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Flash ページを消去する
 * @param address ページ先頭アドレス (ページ境界アラインであること)
 */
void flash_fs_arch_page_erase(uint32_t address);

/**
 * @brief Flash にバイト列を書き込む
 * @param address 書き込み先アドレス
 * @param data    書き込みデータ
 * @param size    バイト数
 *
 * @pre address は4バイトアラインであること
 * @pre data の内容はワードアラインでパディング済みであること (実装依存)
 */
void flash_fs_arch_write(uint32_t address, const void *data, size_t size);

/**
 * @brief Flash からバイト列を読み出す
 * @param address 読み出しアドレス
 * @param buf     読み出し先バッファ
 * @param size    バイト数
 */
void flash_fs_arch_read(uint32_t address, void *buf, size_t size);

#ifdef __cplusplus
}
#endif
