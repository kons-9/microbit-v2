/**
 * @file main.cpp
 * @brief 不揮発ログ読み出し用リカバリーモード
 *
 * 通常のファームウェア更新は行わず、Flash FS に保存されたログを
 * UART shell から読み出す。B ボタンを押した状態で起動すると、
 * 次回起動先を main app に戻して再起動する。
 */

#define LOG_TAG "RECOVERY"
#include "log.h"

#include <utkernel/task>

#include "button.h"
#include "flash.h"
#include "fs.h"
#include "shell.h"
#include "sysconfig.h"
#include "uart.h"

struct Drivers {
    drivers::Uart uart;
    drivers::Flash flash;
    drivers::Button button;
};

struct Config {
    Drivers &drivers;
    fs::FileSystem file_system;

    explicit Config(Drivers &drivers)
        : drivers(drivers)
        , file_system(drivers.flash) {
    }
};

static Drivers s_drivers;
static Config s_config(s_drivers);
static utkernel::task s_shell_task;

static void shell_task(void *) {
    for (;;) {
        shell::poll();
        utkernel::task::sleep_for(10);
    }
}

extern "C" int usermain(void) {
    s_drivers.uart.init();
    logging::Logger::instance().init(logging::LogLevel::Debug, s_drivers.uart);

    s_drivers.button.init();
    if (s_drivers.button.is_pressed(drivers::ButtonId::B)) {
        LOG_I("B pressed: returning to main app");
        sysconfig::reboot(sysconfig::BOOT_APP);
    }

    auto fs_result = s_config.file_system.init();

    if (fs_result != 0) {
        LOG_E("fs init failed: %ld", static_cast<long>(fs_result));
        utkernel::task::sleep_forever();
        return -1;
    }

    shell::init(s_config.drivers.uart, s_config.file_system, nullptr, 0, shell::Mode::ReadOnly);

    utkernel::task::config task_config;
    task_config.priority = 10;
    task_config.stack_size = 2048;
    if (!s_shell_task.create(shell_task, task_config) || !s_shell_task.start()) {
        LOG_E("shell task create/start failed");
        utkernel::task::sleep_forever();
        return -1;
    }

    LOG_I("recovery mode started (read-only shell)");
    utkernel::task::sleep_forever();
    return 0;
}
