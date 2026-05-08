/**
 * @file uart.cpp
 * @brief UART ドライバ共通ディスパッチャ
 */

#include "uart.h"
#include "arch/uart_arch.h"

#include <cstring>

static constexpr uint32_t DEFAULT_BAUDRATE = UART_BAUDRATE_115200;

int32_t uart_init(const UARTConfig *config) {
    uint32_t baud = (config != nullptr) ? config->m_baudrate : DEFAULT_BAUDRATE;
    return uart_arch_init(baud);
}

int32_t uart_write(const uint8_t *data, size_t len) {
    if (data == nullptr || len == 0) {
        return 0;
    }
    return uart_arch_write(data, len);
}

int32_t uart_read(uint8_t *buf, size_t buf_len, uint32_t timeout_ms) {
    if (buf == nullptr || buf_len == 0) {
        return 0;
    }
    return uart_arch_read(buf, buf_len, timeout_ms);
}

int32_t uart_puts(const char *str) {
    if (str == nullptr) {
        return 0;
    }
    auto len = std::strlen(str);
    return uart_write(reinterpret_cast<const uint8_t *>(str), len);
}
