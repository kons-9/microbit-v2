/**
 * @file test_shell.cpp
 * @brief Shell統合テスト
 */

#define LOG_TAG "ITEST"
#include "integration_test.h"

#include "log.h"
#include "shell.h"

#include <cstddef>

namespace integration_test {

namespace {

volatile bool s_custom_cmd_called = false;

void custom_cmd_handler(int32_t argc, const char *const *argv) {
    (void)argc;
    (void)argv;
    s_custom_cmd_called = true;
    shell::puts("custom command executed\r\n");
}

const shell::Command s_test_commands[] = {
    {"itest", "run integration test custom command", custom_cmd_handler},
};

}  // namespace

void run_shell_test(Context &context) {
    LOG_I("=== Shell Test ===");

    shell::init(context.drivers.uart,
                context.config.file_system,
                s_test_commands,
                sizeof(s_test_commands) / sizeof(s_test_commands[0]));

    s_custom_cmd_called = false;
    const char command[] = "itest\r";
    for (size_t i = 0; i < sizeof(command) - 1; i++) {
        shell::feed_char(command[i]);
    }
    context.assert_true(s_custom_cmd_called, "shell::feed_char dispatches");
}

}  // namespace integration_test
