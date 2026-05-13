#pragma once

/**
 * @file uart_arch.h
 * @brief UART アーキテクチャ固有インターフェース (内部用)
 *
 * 各ターゲット (linux, microbit) が実装する。
 */

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

int32_t uart_arch_init(uint32_t baudrate);
int32_t uart_arch_write(const uint8_t *data, size_t len);
int32_t uart_arch_read(uint8_t *buf, size_t buf_len, uint32_t timeout_ms);

#ifdef __cplusplus
}
#endif
