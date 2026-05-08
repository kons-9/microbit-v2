#pragma once

/**
 * @file ota.h
 * @brief OTA (Over-The-Air) Update Interface
 *
 * BLE GATT 経由でファームウェアイメージを受信し、
 * Flash のアプリケーションスロットに書き込む。
 *
 * @pre ota_start_receive() を呼ぶ前に ble_init() が完了していること
 */

#include <stdint.h>
#include <stddef.h>

#include <sysconfig.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ==================================================================
 * Error Codes
 * ================================================================== */

#ifdef __cplusplus
enum class OTAError : int32_t {
    Ok = 0,
    State = -1,    /**< 不正な状態遷移 */
    Checksum = -2, /**< チェックサム不一致 */
    Size = -3,     /**< イメージサイズ超過 */
};
#else
typedef enum {
    OTA_ERROR_OK = 0,
    OTA_ERROR_STATE = -1,
    OTA_ERROR_CHECKSUM = -2,
    OTA_ERROR_SIZE = -3,
} OTAError;
#endif

/* ==================================================================
 * OTA State
 * ================================================================== */

#ifdef __cplusplus
enum class OTAState : uint8_t {
    Idle = 0,      /**< 待機中 */
    Receiving = 1, /**< イメージ受信中 */
    Verifying = 2, /**< チェックサム検証中 */
    Complete = 3,  /**< 完了 */
    Error = 4,     /**< エラー */
};
#else
typedef enum {
    OTA_STATE_IDLE = 0,
    OTA_STATE_RECEIVING = 1,
    OTA_STATE_VERIFYING = 2,
    OTA_STATE_COMPLETE = 3,
    OTA_STATE_ERROR = 4,
} OTAState;
#endif

/* ==================================================================
 * OTA Image Header
 * ================================================================== */

/** OTA イメージマジックナンバー ("OTA1") */
#define OTA_IMAGE_MAGIC 0x4F544131U

typedef struct {
    uint32_t magic;      /**< マジックナンバー (OTA_IMAGE_MAGIC) */
    uint32_t image_size; /**< イメージ本体サイズ (bytes) */
    uint32_t version;    /**< ファームウェアバージョン */
    uint32_t crc32;      /**< イメージ本体の CRC32 */
} OTAImageHeader;

/* ==================================================================
 * OTA Progress Callback
 * ================================================================== */

/**
 * 進捗通知コールバック型
 * @param received_bytes  受信済みバイト数
 * @param total_bytes     イメージ全体のバイト数
 */
typedef void (*OTAProgressCallback)(uint32_t received_bytes, uint32_t total_bytes);

/* ==================================================================
 * API
 * ================================================================== */

/**
 * OTA 受信を開始する (ブロッキング)
 *
 * BLE GATT サービスを起動し、ホストからのイメージ転送を待機する。
 * 受信完了またはエラーで返る。
 *
 * @pre  ble_init() が成功していること
 * @post 受信完了時は OTAState::Complete, エラー時は OTAState::Error になる
 *
 * @return 0 on success, OTAError on failure
 */
int32_t ota_start_receive(void);

/**
 * 進捗コールバックを設定する
 * @param callback  コールバック関数 (NULL で無効化)
 */
void ota_set_progress_callback(OTAProgressCallback callback);

/**
 * 現在の OTA 状態を取得する
 * @return 現在の OTAState
 */
uint8_t ota_get_state(void);

/**
 * ブートモードを切り替えて再起動する
 *
 * @pre  App モードに切り替える場合は OTAState::Complete であること
 * @post 成功時はリセットし、この関数からは返らない
 *
 * @param mode  SYSCONFIG_BOOT_APP or SYSCONFIG_BOOT_UPDATER
 * @return OTAError::State (前提条件未達時のみ返る。成功時は返らない)
 */
int32_t ota_switch_mode(sysconfig_boot_mode_t mode);

/**
 * OTA 状態をリセットする (エラー後のリトライ用)
 */
void ota_reset(void);

/* ==================================================================
 * Internal: GATT コールバックから呼ばれるハンドラ
 * ================================================================== */

/**
 * ヘッダ受信ハンドラ
 * @param header  受信したイメージヘッダ
 * @return 0 on success
 */
int32_t ota_on_header_received(const OTAImageHeader *header);

/**
 * データチャンク受信ハンドラ
 * @param data    受信データ
 * @param length  データ長 (bytes)
 * @return 0 on success
 */
int32_t ota_on_data_received(const uint8_t *data, uint32_t length);

#ifdef __cplusplus
}
#endif
