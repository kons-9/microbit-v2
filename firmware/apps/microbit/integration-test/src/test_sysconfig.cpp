/**
 * @file test_sysconfig.cpp
 * @brief Sysconfig統合テスト
 */

#define LOG_TAG "ITEST"
#include "integration_test.h"

#include "log.h"
#include "sysconfig.h"

namespace integration_test {

void run_sysconfig_test(Context &context) {
    LOG_I("=== Sysconfig Test ===");

    auto settings_addr = sysconfig::get_settings_address();
    LOG_I("settings addr: 0x%lx", settings_addr);
    context.assert_true(settings_addr == 0x7F000, "settings address");

    auto recovery_addr = sysconfig::get_recovery_address();
    LOG_I("recovery addr: 0x%lx", recovery_addr);
    context.assert_true(recovery_addr == 0x6E000, "recovery address");

    auto app_addr = sysconfig::get_app_slot_address();
    LOG_I("app slot addr: 0x%lx", app_addr);
    context.assert_true(app_addr == 0x26000, "app slot address");

    auto app_size = sysconfig::get_app_slot_size();
    LOG_I("app slot size: %lu bytes", app_size);
    context.assert_true(app_size > 0, "app slot size > 0");
}

}  // namespace integration_test
