/**
 * @file main.cpp
 * @brief 非ドライバコンポーネント統合テストのエントリー
 *
 * テスト本体は test_*.cpp に分離し、このファイルでは依存関係の初期化、
 * テストタスク、shellタスクの起動だけを行う。
 */

#define LOG_TAG "ITEST"
#include "integration_test.h"

#include "log.h"
#include <utkernel/task>

namespace {

integration_test::Context s_context;
utkernel::task s_shell_task;
utkernel::task s_main_task;

void shell_task(void *) {
    for (;;) {
        s_context.shell.poll();
        utkernel::task::sleep_for(10);
    }
}

void main_task(void *) {
    LOG_I("========================================");
    LOG_I("  Integration Test Start");
    LOG_I("========================================");

    integration_test::run_uart_test(s_context);
    integration_test::run_sysconfig_test(s_context);
    integration_test::run_fs_test(s_context);
    integration_test::run_ble_test(s_context);
    integration_test::run_shell_test(s_context);

    LOG_I("========================================");
    LOG_I("  Results: %lu passed, %lu failed", s_context.pass_count, s_context.fail_count);
    LOG_I("========================================");

    if (s_context.fail_count == 0) {
        LOG_I("ALL TESTS PASSED");
    } else {
        LOG_E("SOME TESTS FAILED");
    }

    LOG_I("Shell task starting... (type 'help' for commands)");
    utkernel::task::config shell_config;
    shell_config.name = "itest_shell";
    shell_config.priority = 10;
    shell_config.stack_size = 2048;
    if (!s_shell_task.create(shell_task, shell_config) || !s_shell_task.start()) {
        LOG_E("shell task create/start failed");
    }
}

}  // namespace

extern "C" int usermain(void) {
    s_context.drivers.uart.init();
    logging::Logger::instance().init(logging::LogLevel::Debug, s_context.drivers.uart);

    utkernel::task::config main_config;
    main_config.name = "itest_main";
    main_config.priority = 8;
    main_config.stack_size = 4096;
    if (!s_main_task.create(main_task, main_config) || !s_main_task.start()) {
        LOG_E("main task create/start failed");
        return -1;
    }

    utkernel::task::sleep_forever();
    return 0;
}
