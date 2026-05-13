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

#include "io_stream.h"

#include <cstdint>
#include <cstddef>

namespace drivers {

static constexpr uint32_t UART_BAUDRATE_9600 = 9600;
static constexpr uint32_t UART_BAUDRATE_115200 = 115200;

/**
 * @brief UART ドライバ (io::Stream 実装)
 */
class Uart : public io::Stream {
  public:
    /**
     * UART を初期化する
     * @param baudrate ボーレート (デフォルト 115200)
     * @return 0: 成功
     */
    int32_t init(uint32_t baudrate = UART_BAUDRATE_115200);

    int32_t write(const uint8_t *data, size_t len) override;
    int32_t read(uint8_t *buf, size_t buf_len, uint32_t timeout_ms) override;

    /**
     * 文字列を送信する
     * @param str NULL終端文字列
     * @return 送信バイト数
     */
    int32_t puts(const char *str);
};

}  // namespace drivers
