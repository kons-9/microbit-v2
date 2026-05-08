/**
 * @file mock_uart.cpp
 * @brief テスト用 UART モック — 出力をバッファにキャプチャ
 */

#include "uart.h"

#include <cstring>
#include <cstdio>

static char s_outputBuf[4096];
static size_t s_outputPos = 0;

/* テストヘルパ: 出力バッファをリセット */
extern "C" void mock_uart_reset() {
    s_outputPos = 0;
    s_outputBuf[0] = '\0';
}

/* テストヘルパ: 出力バッファ取得 */
extern "C" const char *mock_uart_get_output() {
    s_outputBuf[s_outputPos] = '\0';
    return s_outputBuf;
}

/* テストヘルパ: 出力バッファ長取得 */
extern "C" size_t mock_uart_get_output_len() {
    return s_outputPos;
}

/* UART API モック実装 */

int32_t uart_init(const UARTConfig * /*config*/) {
    mock_uart_reset();
    return 0;
}

int32_t uart_write(const uint8_t *data, size_t len) {
    size_t space = sizeof(s_outputBuf) - 1 - s_outputPos;
    size_t to_copy = (len < space) ? len : space;
    std::memcpy(s_outputBuf + s_outputPos, data, to_copy);
    s_outputPos += to_copy;
    return static_cast<int32_t>(to_copy);
}

int32_t uart_read(uint8_t * /*buf*/, size_t /*buf_len*/, uint32_t /*timeout_ms*/) {
    return 0;
}

int32_t uart_puts(const char *str) {
    size_t len = std::strlen(str);
    return uart_write(reinterpret_cast<const uint8_t *>(str), len);
}
