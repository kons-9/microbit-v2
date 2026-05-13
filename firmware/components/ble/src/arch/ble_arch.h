#pragma once

/**
 * @file ble_arch.h
 * @brief BLE アーキテクチャ固有インターフェース (内部用)
 *
 * 各ターゲット (linux, microbit) が実装する。
 * src 内部でのみ使用する。
 */

#include <cstdint>

#include "ble.h"

/**
 * arch 層 → 共通層へのイベント通知コールバック型
 *
 * arch 実装がパケットを受信したとき、このコールバックで上位に通知する。
 * ble.cpp 側で登録する。
 */
using BLEArchOnAdvertiseCallback = void (*)(const ble::DiscoveryDescriptor *descriptor);

/**
 * HW を初期化する
 * @return 0 on success
 */
int32_t ble_arch_init(void);

/**
 * スキャンを開始する
 *
 * @param interval_625us  スキャン間隔 (0.625ms 単位)
 * @param window_625us    スキャンウィンドウ (0.625ms 単位)
 * @param is_passive      1=passive scan
 * @param on_advertise    ADV パケット受信時のコールバック (arch → ble.cpp)
 * @return 0 on success
 */
int32_t ble_arch_scan_start(uint16_t interval_625us,
                            uint16_t window_625us,
                            int32_t is_passive,
                            BLEArchOnAdvertiseCallback on_advertise);

/**
 * スキャンを停止する
 * @return 0 on success
 */
int32_t ble_arch_scan_stop(void);

/* ---- Advertise (Broadcaster) ---- */

/**
 * ADV パケットの PDU を構築済みバッファとして設定する
 *
 * @param pdu       PDU バッファ (S0 + LENGTH + Payload の形式)
 * @param pdu_length バッファ全体のバイト数
 * @return 0 on success
 */
int32_t ble_arch_advertise_set_pdu(const uint8_t *pdu, uint8_t pdu_length);

/**
 * Advertising を開始する (ch37→38→39 を巡回送信)
 *
 * @param interval_625us  送信間隔 (0.625ms 単位)
 * @return 0 on success
 */
int32_t ble_arch_advertise_start(uint16_t interval_625us);

/**
 * Advertising を停止する
 * @return 0 on success
 */
int32_t ble_arch_advertise_stop(void);
