#pragma once

/**
 * OTA (Over-The-Air) Update Interface
 *
 * BLE GATT経由でファームウェアイメージを受信し、
 * Flashのアプリケーションスロットに書き込む。
 */

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ---- Error Codes ---- */

#define OTA_OK 0
#define OTA_ERR_STATE -1    /**< 不正な状態遷移 */
#define OTA_ERR_CHECKSUM -2 /**< チェックサム不一致 */
#define OTA_ERR_SIZE -3     /**< イメージサイズ超過 */

/* ---- Boot Mode ---- */

typedef enum {
    OTA_BOOT_APP = 0, /**< メインアプリで起動 */
    OTA_BOOT_UPDATER, /**< updaterモードで起動 (未書き込み/不正値時のデフォルト) */
} ota_boot_mode_t;

/* ---- OTA State ---- */

typedef enum {
    OTA_STATE_IDLE = 0,  /**< 待機中 */
    OTA_STATE_RECEIVING, /**< イメージ受信中 */
    OTA_STATE_VERIFYING, /**< チェックサム検証中 */
    OTA_STATE_COMPLETE,  /**< 完了 */
    OTA_STATE_ERROR,     /**< エラー */
} ota_state_t;

/* ---- OTA Image Header ---- */

typedef struct {
    uint32_t magic;      /**< マジックナンバー (OTA_IMAGE_MAGIC) */
    uint32_t image_size; /**< イメージ本体サイズ (bytes) */
    uint32_t version;    /**< ファームウェアバージョン */
    uint32_t crc32;      /**< イメージ本体のCRC32 */
} ota_image_header_t;

#define OTA_IMAGE_MAGIC 0x4F544131 /* "OTA1" */

/* ---- OTA Progress Callback ---- */

/**
 * 進捗通知コールバック型
 * @param received  受信済みバイト数
 * @param total     イメージ全体のバイト数
 */
typedef void (*ota_progress_fn)(uint32_t received, uint32_t total);

/* ---- API ---- */

/**
 * OTA受信開始 (ブロッキング)
 *
 * BLE GATTサービスを起動し、ホストからのイメージ転送を待機。
 * 受信完了またはエラーで返る。
 *
 * @return OTA_OK on success, OTA_ERR_xxx on failure
 */
int ota_start_receive(void);

/**
 * 進捗コールバックを設定
 * @param fn  コールバック関数 (NULL で無効化)
 */
void ota_set_progress_callback(ota_progress_fn fn);

/**
 * 現在のOTA状態を取得
 */
ota_state_t ota_get_state(void);

/**
 * ブートモードを切り替えて再起動
 *
 * OTA_BOOT_APP: OTA_STATE_COMPLETE時のみ許可。成功時はリセットし返らない。
 * OTA_BOOT_UPDATER: 常に許可。成功時はリセットし返らない。
 *
 * @param mode  OTA_BOOT_APP or OTA_BOOT_UPDATER
 * @return OTA_ERR_STATE (前提条件未達時のみ返る。成功時は返らない)
 */
int ota_switch_mode(ota_boot_mode_t mode);

/**
 * OTA状態をリセット (エラー後のリトライ用)
 */
void ota_reset(void);

#ifdef __cplusplus
}
#endif
