#pragma once

/**
 * @file beacon.h
 * @brief BLE ビーコン制御モジュール
 *
 * BLE Advertising を使ったビーコン発信と、
 * UART シェルからの制御コマンドを提供する。
 */

#include <cstdint>
#include "shell.h"

namespace beacon {

/* ================================================================== */
/*  Types                                                             */
/* ================================================================== */

/** ビーコン設定 */
struct Config {
    uint16_t interval_ms;  /**< Advertising 間隔 [ms] (デフォルト: 1000) */
    int8_t tx_power;       /**< TX パワー [dBm] (AD データ用, デフォルト: 0) */
    uint8_t company_id_lo; /**< Company ID 下位バイト */
    uint8_t company_id_hi; /**< Company ID 上位バイト */
};

/* ================================================================== */
/*  API                                                               */
/* ================================================================== */

/**
 * @brief ビーコンモジュールを初期化する
 *
 * BLE を初期化し、シェルコマンドを登録する。
 * start() を呼ぶまで Advertising は開始しない。
 *
 * @param config  ビーコン設定 (nullptr の場合はデフォルト値)
 * @return 0: 成功, 負値: エラー
 */
int32_t init(const Config *config);

/**
 * @brief ビーコン発信を開始する
 * @return 0: 成功, 負値: エラー
 */
int32_t start(void);

/**
 * @brief ビーコン発信を停止する
 * @return 0: 成功
 */
int32_t stop(void);

/**
 * @brief ビーコンが発信中かどうか
 * @return 1: 発信中, 0: 停止中
 */
int32_t is_active(void);

/**
 * @brief Advertising 間隔を変更する
 * @param interval_ms  間隔 [ms] (20〜10240)
 * @return 0: 成功, 負値: パラメータエラー
 *
 * @note 発信中の場合は再起動して反映する
 */
int32_t set_interval(uint16_t interval_ms);

/**
 * @brief 現在の Advertising 間隔を取得する
 * @return 間隔 [ms]
 */
uint16_t get_interval(void);

/**
 * @brief シェルコマンドテーブルを取得する
 * @param[out] count  コマンド数
 * @return コマンドテーブルへのポインタ
 */
const shell::Command *get_shell_commands(uint8_t *count);

}  // namespace beacon
