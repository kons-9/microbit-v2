/**
 * @file uart.cpp
 * @brief UART ドライバ共通ディスパッチャ
 */

#include "uart.h"
#include "arch/uart_arch.h"

#include "log.h"

#include <cstring>

namespace drivers {

int32_t Uart::init(uint32_t baudrate) {
    return uart_arch_init(baudrate);
}

int32_t Uart::write(const uint8_t *data, size_t len) {
    if (data == nullptr || len == 0) {
        return 0;
    }
    return uart_arch_write(data, len);
}

int32_t Uart::read(uint8_t *buf, size_t buf_len, uint32_t timeout_ms) {
    if (buf == nullptr || buf_len == 0) {
        return 0;
    }
    LOG_I("uart read: buf_len=%lu, timeout=%lu ms", static_cast<uint32_t>(buf_len), timeout_ms);
    return uart_arch_read(buf, buf_len, timeout_ms);
}

int32_t Uart::puts(const char *str) {
    if (str == nullptr) {
        return 0;
    }
    auto len = std::strlen(str);
    return write(reinterpret_cast<const uint8_t *>(str), len);
}

}  // namespace drivers
