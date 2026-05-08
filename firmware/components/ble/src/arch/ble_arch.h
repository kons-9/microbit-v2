#pragma once

/**
 * BLE Arch-specific internal interface
 *
 * 各ターゲット (linux, microbit) が実装する。
 * src 内部でのみ使用。
 */

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** HW 初期化 */
int ble_arch_init(void);

/** スキャン開始 */
int ble_arch_scan_start(uint16_t interval_625us, uint16_t window_625us, int passive);

/** スキャン停止 */
int ble_arch_scan_stop(void);

#ifdef __cplusplus
}
#endif
