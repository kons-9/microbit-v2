/**
 * @file uart_linux.cpp
 * @brief Linux UART 実装 (stdout/stdin)
 */

#include "arch/uart_arch.h"

#include <cstdio>

int32_t uart_arch_init(uint32_t baudrate) {
    (void)baudrate;
    return 0;
}

int32_t uart_arch_write(const uint8_t *data, size_t len) {
    return static_cast<int32_t>(std::fwrite(data, 1, len, stdout));
}

int32_t uart_arch_read(uint8_t *buf, size_t buf_len, uint32_t timeout_ms) {
    (void)timeout_ms;
    return static_cast<int32_t>(std::fread(buf, 1, buf_len, stdin));
}
