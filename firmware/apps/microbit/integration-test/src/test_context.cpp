/**
 * @file test_context.cpp
 * @brief 統合テストの共通コンテキスト
 */

#define LOG_TAG "ITEST"
#include "integration_test.h"

#include "log.h"

namespace integration_test {

void Context::assert_true(bool condition, const char *message) {
    if (condition) {
        LOG_I("[PASS] %s", message);
        pass_count++;
    } else {
        LOG_E("[FAIL] %s", message);
        fail_count++;
    }
}

}  // namespace integration_test
