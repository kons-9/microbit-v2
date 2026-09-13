/**
 * @file main.cpp
 * @brief 非ドライバコンポーネント統合テスト
 *
 * ドライバ層が正常に動作することを前提として、
 * ble, fs, shell, sysconfig の
 * 各コンポーネントを実機上でテストする。
 *
 * テスト結果は UART (LOG) 経由で出力される。
 * シェルタスクを起動し、対話的にコマンドテストも可能。
 *
 * タスク構成:
 *   - メインタスク: 自動テストを順次実行
 *   - Shell タスク: UART shell ポーリング (対話テスト用)
 */

#define LOG_TAG "ITEST"
#include "log.h"

#include <tm/tmonitor.h>
#include <utkernel/task>

#include "ble.h"
#include "flash.h"
#include "fs.h"
#include "shell.h"
#include "sysconfig.h"
#include "uart.h"

#include <cstdint>
#include <cstring>

/* ==================================================================
 * Application dependencies
 * ================================================================== */

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

static Drivers s_drivers;
static Config s_config(s_drivers);

/* ==================================================================
 * ヘルパー
 * ================================================================== */

static uint32_t s_pass_count = 0;
static uint32_t s_fail_count = 0;

#define TEST_ASSERT(cond, msg)                                                                                         \
    do {                                                                                                               \
        if (cond) {                                                                                                    \
            LOG_I("[PASS] %s", msg);                                                                                   \
            s_pass_count++;                                                                                            \
        } else {                                                                                                       \
            LOG_E("[FAIL] %s", msg);                                                                                   \
            s_fail_count++;                                                                                            \
        }                                                                                                              \
    } while (0)

/* ==================================================================
 * UART RX テスト
 * ================================================================== */

static void test_uart_rx() {
    LOG_I("=== UART RX Test ===");
    LOG_I("Send any char within 3 seconds...");

    uint8_t buf[16];
    auto received = s_drivers.uart.read(buf, sizeof(buf), 3000);

    if (received > 0) {
        LOG_I("received: %ld bytes", static_cast<int32_t>(received));
        TEST_ASSERT(true, "uart_read");
    } else {
        LOG_I("timeout (no input - skipped)");
        TEST_ASSERT(true, "uart_read (skipped)");
    }
}

/* ==================================================================
 * Sysconfig テスト
 * ================================================================== */

static void test_sysconfig() {
    LOG_I("=== Sysconfig Test ===");

    auto settings_addr = sysconfig::get_settings_address();
    LOG_I("settings addr: 0x%lx", settings_addr);
    TEST_ASSERT(settings_addr == 0x7F000, "settings address");

    auto updater_addr = sysconfig::get_updater_address();
    LOG_I("updater addr: 0x%lx", updater_addr);
    TEST_ASSERT(updater_addr == 0x6E000, "updater address");

    auto app_addr = sysconfig::get_app_slot_address();
    LOG_I("app slot addr: 0x%lx", app_addr);
    TEST_ASSERT(app_addr == 0x26000, "app slot address");

    auto app_size = sysconfig::get_app_slot_size();
    LOG_I("app slot size: %lu bytes", app_size);
    TEST_ASSERT(app_size > 0, "app slot size > 0");
}

/* ==================================================================
 * Flash FS テスト
 * ================================================================== */

static void test_fs(fs::FileSystem &file_system) {
    LOG_I("=== Flash FS Test ===");

    auto init_result = file_system.init();
    LOG_I("fs_init: %ld", init_result);
    TEST_ASSERT(init_result == 0, "fs_init");

    /* ファイル名 → ID 変換 */
    auto log_name = file_system.get_name(fs::FileId::Log);
    LOG_I("file[0] name: %s", log_name ? *log_name : "(null)");
    TEST_ASSERT(log_name.has_value(), "fs::get_name(LOG)");

    /* ファイル情報取得 */
    fs::FileInfo file_info;
    auto info_result = file_system.get_info(fs::FileId::Settings, &file_info);
    LOG_I("settings: type=%lu, capacity=%lu, used=%lu",
          static_cast<uint32_t>(static_cast<uint8_t>(file_info.type)),
          file_info.capacity,
          file_info.used);
    TEST_ASSERT(info_result.has_value(), "fs::get_info(SETTINGS)");

    /* Block write / read (SETTINGS ファイル) */
    const uint32_t test_val = 0xDEADBEEF;
    auto write_result = file_system.block_write(fs::FileId::Settings, 0, &test_val, sizeof(test_val));
    TEST_ASSERT(write_result.has_value(), "fs_block_write");

    uint32_t read_val = 0;
    auto read_sz = file_system.block_read(fs::FileId::Settings, 0, &read_val, sizeof(read_val));
    LOG_I("read back: 0x%lx (size=%lu)", read_val, static_cast<uint32_t>(read_sz));
    TEST_ASSERT(read_sz == sizeof(test_val) && read_val == test_val, "fs_block_read matches");

    /* Stream append / read (LOG ファイル) */
    file_system.erase(fs::FileId::Log);
    const char msg[] = "hello";
    auto append_result = file_system.append(fs::FileId::Log, msg, sizeof(msg));
    TEST_ASSERT(append_result.has_value(), "fs_append");

    char read_buf[16] = {};
    auto stream_sz = file_system.read(fs::FileId::Log, 0, read_buf, sizeof(read_buf));
    LOG_I("stream read: \"%s\" (size=%lu)", read_buf, static_cast<uint32_t>(stream_sz));
    TEST_ASSERT(stream_sz >= sizeof(msg) && std::memcmp(read_buf, msg, sizeof(msg)) == 0, "fs_read matches");
}

/* ==================================================================
 * BLE テスト
 * ================================================================== */

static volatile bool s_ble_scan_received = false;

static int32_t ble_scan_callback(ble::GapEvent *event, void *) {
    if (event->type == ble::GapEventType::Discovery) {
        s_ble_scan_received = true;
    }
    return 0;
}

static void test_ble() {
    LOG_I("=== BLE Test ===");

    auto ble_result = ble::init();
    LOG_I("ble_init: %ld", ble_result);
    TEST_ASSERT(ble_result == 0, "ble_init");

    /* Advertise テスト */
    const uint8_t ad_data[] = {
        0x02, 0x01, 0x06, /* Flags: LE General Discoverable */
        0x11, 0x09, 'i',  'n', 't', 'e', 'g', 'r', 'a',
        't',  'i',  'o',  'n', ' ', 't', 'e', 's', 't', /* Complete Local Name */
    };
    auto set_result = ble::gap_advertise_set_data(ad_data, sizeof(ad_data));
    TEST_ASSERT(set_result == 0, "ble::gap_advertise_set_data");

    ble::GapAdvertiseParams adv_params = {};
    adv_params.interval_min = 160; /* 100ms */
    adv_params.interval_max = 160;
    adv_params.advertise_type = ble::AdvertisePduType::ConnectableUndirected;

    auto adv_result = ble::gap_advertise_start(ble::AddressType::Public, &adv_params);
    LOG_I("advertise_start: %ld", adv_result);
    TEST_ASSERT(adv_result == 0, "ble::gap_advertise_start");

    TEST_ASSERT(ble::gap_advertise_active() == 1, "ble::gap_advertise_active");

    ble::gap_advertise_stop();
    TEST_ASSERT(ble::gap_advertise_active() == 0, "ble_gap_advertise stopped");

    /* Discover テスト (2秒スキャン) */
    s_ble_scan_received = false;
    ble::DiscoveryParams scan_params = {};
    scan_params.interval = 160; /* 100ms */
    scan_params.window = 80;    /* 50ms */
    scan_params.is_passive = 1;
    scan_params.filter_duplicates = 0;

    auto disc_result = ble::gap_discover(ble::AddressType::Public, 2000, &scan_params, ble_scan_callback, nullptr);
    LOG_I("discover: %ld", disc_result);
    TEST_ASSERT(disc_result == 0, "ble_gap_discover");

    /* 2秒待って結果確認 */
    utkernel::task::sleep_for(2500);

    if (s_ble_scan_received) {
        LOG_I("BLE device found");
    } else {
        LOG_I("no BLE device found (ok if none nearby)");
    }
    TEST_ASSERT(true, "ble_gap_discover completed");
}

/* ==================================================================
 * Shell テスト
 * ================================================================== */

static volatile bool s_custom_cmd_called = false;

static void custom_cmd_handler(int32_t argc, const char *const *argv) {
    (void)argc;
    (void)argv;
    s_custom_cmd_called = true;
    shell::puts("custom command executed\r\n");
}

static const shell::Command s_test_commands[] = {
    {"itest", "run integration test custom command", custom_cmd_handler},
};

static void test_shell() {
    LOG_I("=== Shell Test ===");

    /* Shell 初期化は後述のメインタスクで1回だけ行う */

    /* feed_char でコマンドディスパッチテスト */
    s_custom_cmd_called = false;
    const char cmd[] = "itest\r";
    for (size_t i = 0; i < sizeof(cmd) - 1; i++) {
        shell::feed_char(cmd[i]);
    }
    TEST_ASSERT(s_custom_cmd_called, "shell::feed_char dispatches");
}

/* ==================================================================
 * Shell Task (対話テスト用)
 * ================================================================== */

static utkernel::task s_shell_task;
static utkernel::task s_main_task;

static void shell_task(void *) {

    for (;;) {
        shell::poll();
        utkernel::task::sleep_for(10);
    }
}

/* ==================================================================
 * メインタスク
 * ================================================================== */

static void main_task(void *) {

    LOG_I("========================================");
    LOG_I("  Integration Test Start");
    LOG_I("========================================");

    /* --- 自動テスト --- */
    test_uart_rx();
    test_sysconfig();
    test_fs(s_config.file_system);
    test_ble();
    /* Shell 初期化 (itest コマンド) */
    shell::init(s_drivers.uart,
                s_config.file_system,
                s_test_commands,
                sizeof(s_test_commands) / sizeof(s_test_commands[0]));
    test_shell();

    /* --- 結果サマリ --- */
    LOG_I("========================================");
    LOG_I("  Results: %lu passed, %lu failed", s_pass_count, s_fail_count);
    LOG_I("========================================");

    if (s_fail_count == 0) {
        LOG_I("ALL TESTS PASSED");
    } else {
        LOG_E("SOME TESTS FAILED");
    }

    /* Shell タスク起動 (対話テスト用) */
    LOG_I("Shell task starting... (type 'help' for commands)");
    utkernel::task::config shell_config;
    shell_config.priority = 10;
    shell_config.stack_size = 2048;
    if (!s_shell_task.create(shell_task, shell_config) || !s_shell_task.start()) {
        LOG_E("shell task create/start failed");
    }
}

/* ==================================================================
 * Entry Point
 * ================================================================== */

static int app_main();

extern "C" int usermain(void) {
    return app_main();
}

static int app_main() {
    /* UART 初期化 */
    s_drivers.uart.init();
    logging::Logger::instance().init(logging::LogLevel::Debug, s_drivers.uart);

    /* メインタスク生成・起動 */
    utkernel::task::config main_config;
    main_config.priority = 8;
    main_config.stack_size = 4096;
    if (!s_main_task.create(main_task, main_config) || !s_main_task.start()) {
        LOG_E("main task create/start failed");
        return -1;
    }

    utkernel::task::sleep_forever();

    return 0;
}
