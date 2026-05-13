#pragma once

/**
 * @file ota.h
 * @brief OTA (Over-The-Air) Update Interface
 *
 * BLE GATT 経由でファームウェアイメージを受信し、
 * Flash のアプリケーションスロットに書き込む。
 *
 * @pre ota::start_receive() を呼ぶ前に ble::init() が完了していること
 */

#include <cstdint>
#include <cstddef>

#include <sysconfig.h>

namespace ota {

/* ==================================================================
 * Error Codes
 * ================================================================== */

enum class Error : int32_t {
    Ok = 0,
    State = -1,    /**< 不正な状態遷移 */
    Checksum = -2, /**< チェックサム不一致 */
    Size = -3,     /**< イメージサイズ超過 */
};

/* ==================================================================
 * OTA State
 * ================================================================== */

enum class State : uint8_t {
    Idle = 0,      /**< 待機中 */
    Receiving = 1, /**< イメージ受信中 */
    Verifying = 2, /**< チェックサム検証中 */
    Complete = 3,  /**< 完了 */
    Error = 4,     /**< エラー */
};

/* ==================================================================
 * OTA Image Header
 * ================================================================== */

/** OTA イメージマジックナンバー ("OTA1") */
constexpr uint32_t IMAGE_MAGIC = 0x4F544131U;

struct ImageHeader {
    uint32_t magic;      /**< マジックナンバー (IMAGE_MAGIC) */
    uint32_t image_size; /**< イメージ本体サイズ (bytes) */
    uint32_t version;    /**< ファームウェアバージョン */
    uint32_t crc32;      /**< イメージ本体の CRC32 */
};

/* ==================================================================
 * OTA Progress Callback
 * ================================================================== */

/**
 * 進捗通知コールバック型
 * @param received_bytes  受信済みバイト数
 * @param total_bytes     イメージ全体のバイト数
 */
using ProgressCallback = void (*)(uint32_t received_bytes, uint32_t total_bytes);

/* ==================================================================
 * API
 * ================================================================== */

/**
 * OTA 受信を開始する (ブロッキング)
 *
 * BLE GATT サービスを起動し、ホストからのイメージ転送を待機する。
 * 受信完了またはエラーで返る。
 *
 * @pre  ble::init() が成功していること
 * @post 受信完了時は State::Complete, エラー時は State::Error になる
 *
 * @return 0 on success, Error on failure
 */
int32_t start_receive(void);

/**
 * 進捗コールバックを設定する
 * @param callback  コールバック関数 (NULL で無効化)
 */
void set_progress_callback(ProgressCallback callback);

/**
 * 現在の OTA 状態を取得する
 * @return 現在の State
 */
uint8_t get_state(void);

/**
 * ブートモードを切り替えて再起動する
 *
 * @pre  App モードに切り替える場合は State::Complete であること
 * @post 成功時はリセットし、この関数からは返らない
 *
 * @param mode  SYSCONFIG_BOOT_APP or SYSCONFIG_BOOT_UPDATER
 * @return Error::State (前提条件未達時のみ返る。成功時は返らない)
 */
int32_t switch_mode(sysconfig_boot_mode_t mode);

/**
 * OTA 状態をリセットする (エラー後のリトライ用)
 */
void reset(void);

/* ==================================================================
 * Internal: GATT コールバックから呼ばれるハンドラ
 * ================================================================== */

/**
 * ヘッダ受信ハンドラ
 * @param header  受信したイメージヘッダ
 * @return 0 on success
 */
int32_t on_header_received(const ImageHeader *header);

/**
 * データチャンク受信ハンドラ
 * @param data    受信データ
 * @param length  データ長 (bytes)
 * @return 0 on success
 */
int32_t on_data_received(const uint8_t *data, uint32_t length);

}  // namespace ota
