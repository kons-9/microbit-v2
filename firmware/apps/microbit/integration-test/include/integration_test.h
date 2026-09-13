#pragma once

#include "flash.h"
#include "fs.h"
#include "uart.h"

#include <cstdint>

namespace integration_test {

struct Drivers {
    drivers::Uart uart;
    drivers::Flash flash;
};

struct Config {
    Drivers &drivers;
    fs::FileSystem file_system;

    explicit Config(Drivers &drivers)
        : drivers(drivers)
        , file_system(drivers.flash) {
    }
};

struct Context {
    Drivers drivers;
    Config config;
    uint32_t pass_count = 0;
    uint32_t fail_count = 0;

    Context()
        : config(drivers) {
    }

    void assert_true(bool condition, const char *message);
};

void run_uart_test(Context &context);
void run_sysconfig_test(Context &context);
void run_fs_test(Context &context);
void run_ble_test(Context &context);
void run_shell_test(Context &context);

}  // namespace integration_test
