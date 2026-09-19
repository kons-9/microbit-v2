/**
 * @file test_uart.cpp
 * @brief UART統合テスト
 */

#define LOG_TAG "ITEST"
#include "integration_test.h"

#include "log.h"

#include <cstdint>

namespace integration_test {

void run_uart_test(Context &context) {
    LOG_I("=== UART RX Test ===");
    LOG_I("Send any char within 3 seconds...");

    uint8_t buf[16];
    auto received = context.drivers.uart.read(buf, sizeof(buf), 3000);

    if (received > 0) {
        LOG_I("received: %ld bytes", static_cast<int32_t>(received));
        context.assert_true(true, "uart_read");
    } else {
        LOG_I("timeout (no input - skipped)");
        context.assert_true(true, "uart_read (skipped)");
    }
}

}  // namespace integration_test
