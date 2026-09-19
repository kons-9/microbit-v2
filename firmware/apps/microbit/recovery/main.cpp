/**
 * @file main.cpp
 * @brief 不揮発ログ読み出し用リカバリーモード
 *
 * 通常のファームウェア更新は行わず、Flash FS に保存されたログを
 * UART shell から読み出す。B ボタンを押した状態で起動すると、
 * 次回起動先を main app に戻して再起動する。
 */

#define LOG_TAG "RECOVERY"
#include "crash_info.h"
#include "log.h"

#include <utkernel/task>

#include <new>

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
    shell::Shell shell;

    explicit Config(Drivers &drivers)
        : drivers(drivers)
        , file_system(drivers.flash) {
    }
};

/*
 * recovery does not retain .init_array because the complete C++ runtime does
 * not fit in the recovery slot. Construct only the objects needed here and
 * do not register destructors; recovery never returns from usermain().
 */
alignas(Drivers) static unsigned char s_drivers_storage[sizeof(Drivers)];
alignas(Config) static unsigned char s_config_storage[sizeof(Config)];

static Drivers *s_drivers;
static Config *s_config;

static void initialize_objects() {
    s_drivers = ::new (s_drivers_storage) Drivers();
    s_config = ::new (s_config_storage) Config(*s_drivers);
}

static void report_previous_crash() {
    const auto *source = reinterpret_cast<const volatile uint32_t *>(sysconfig::get_settings_address() + 4);
    if (source[0] != crash::INFO_MAGIC) {
        return;
    }

    crash::Info info{};
    auto *words = reinterpret_cast<uint32_t *>(&info);
    constexpr uint32_t word_count = sizeof(info) / sizeof(uint32_t);
    for (uint32_t i = 0; i < word_count; i++) {
        words[i] = source[i];
    }

    LOG_E("previous crash: type=%lu pc=0x%08lx lr=0x%08lx sp=0x%08lx",
          static_cast<unsigned long>(info.fault_type),
          static_cast<unsigned long>(info.pc),
          static_cast<unsigned long>(info.lr),
          static_cast<unsigned long>(info.sp));
    LOG_E("fault status: hfsr=0x%08lx cfsr=0x%08lx mmfar=0x%08lx bfar=0x%08lx",
          static_cast<unsigned long>(info.hfsr),
          static_cast<unsigned long>(info.cfsr),
          static_cast<unsigned long>(info.mmfar),
          static_cast<unsigned long>(info.bfar));
}

extern "C" int usermain(void) {
    initialize_objects();

    s_drivers->uart.init();
    logging::Logger::instance().init(logging::LogLevel::Debug, s_drivers->uart);

    report_previous_crash();

    s_drivers->button.init();
    if (s_drivers->button.is_pressed(drivers::ButtonId::B)) {
        LOG_I("B pressed: returning to main app");
        sysconfig::reboot(sysconfig::BOOT_APP);
    }

    auto fs_result = s_config->file_system.init();

    if (fs_result != 0) {
        LOG_E("fs init failed: %ld", static_cast<long>(fs_result));
        utkernel::task::sleep_forever();
        return -1;
    }

    s_config->shell.init(s_config->drivers.uart, s_config->file_system, nullptr, 0, shell::Mode::ReadOnly);

    LOG_I("recovery mode started (read-only shell)");

    for (;;) {
        s_config->shell.poll();
    }

    return 0;
}
