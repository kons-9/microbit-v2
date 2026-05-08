#pragma once

/**
 * @file uart.h
 * @brief UART ドライバ
 *
 * micro:bit v2.2: nRF52833 UARTE0 (USB CDC 経由)
 * - TX: P0.06 (USB Interface MCU 経由)
 * - RX: P1.08 (USB Interface MCU 経由)
 * - デフォルトボーレート: 115200
 */

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

static constexpr uint32_t UART_BAUDRATE_9600 = 9600;
static constexpr uint32_t UART_BAUDRATE_115200 = 115200;

/** UART 設定 */
typedef struct {
    uint32_t m_baudrate;
} UARTConfig;

/**
 * UART を初期化する
 * @param config  設定 (nullptr の場合はデフォルト 115200bps)
 * @return 0: 成功
 */
int32_t uart_init(const UARTConfig *config);

/**
 * データを送信する
 * @param data  送信データ
 * @param len   データ長 [bytes]
 * @return 送信バイト数, 負値はエラー
 */
int32_t uart_write(const uint8_t *data, size_t len);

/**
 * データを受信する (ブロッキング)
 * @param buf         受信バッファ
 * @param buf_len     バッファサイズ
 * @param timeout_ms  タイムアウト [ms], 0=無期限
 * @return 受信バイト数, 負値はエラー
 */
int32_t uart_read(uint8_t *buf, size_t buf_len, uint32_t timeout_ms);

/**
 * 文字列を送信する (NULL 終端)
 * @param str  送信文字列
 * @return 送信バイト数, 負値はエラー
 */
int32_t uart_puts(const char *str);

#ifdef __cplusplus
}
#endif
