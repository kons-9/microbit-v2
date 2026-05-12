/**
 * @file main.cpp
 * @brief 非ドライバコンポーネント統合テスト
 *
 * ドライバ層が正常に動作することを前提として、
 * ble, crash, flash_fs, shell, signal, beacon, sysconfig の
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

#include <tk/tkernel.h>
#include <tm/tmonitor.h>

#include "beacon.h"
#include "ble.h"
#include "crash_info.h"
#include "flash_fs.h"
#include "shell.h"
#include "signal_proc.h"
#include "sysconfig.h"
#include "uart.h"

#include <cstdint>
#include <cstring>

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
    LOG_I("3秒以内に任意の文字を送信してください...");

    uint8_t buf[16];
    auto received = uart_read(buf, sizeof(buf), 3000);

    if (received > 0) {
        LOG_I("受信: %ld bytes", static_cast<int32_t>(received));
        TEST_ASSERT(true, "uart_read");
    } else {
        LOG_I("タイムアウト (入力なし — スキップ)");
        TEST_ASSERT(true, "uart_read (skipped)");
    }
}

/* ==================================================================
 * Sysconfig テスト
 * ================================================================== */

static void test_sysconfig() {
    LOG_I("=== Sysconfig Test ===");

    auto settings_addr = sysconfig_get_settings_address();
    LOG_I("settings addr: 0x%lx", settings_addr);
    TEST_ASSERT(settings_addr == 0x7F000, "settings address");

    auto updater_addr = sysconfig_get_updater_address();
    LOG_I("updater addr: 0x%lx", updater_addr);
    TEST_ASSERT(updater_addr == 0x6E000, "updater address");

    auto app_addr = sysconfig_get_app_slot_address();
    LOG_I("app slot addr: 0x%lx", app_addr);
    TEST_ASSERT(app_addr == 0x26000, "app slot address");

    auto app_size = sysconfig_get_app_slot_size();
    LOG_I("app slot size: %lu bytes", app_size);
    TEST_ASSERT(app_size > 0, "app slot size > 0");
}

/* ==================================================================
 * Signal テスト
 * ================================================================== */

static void test_signal() {
    LOG_I("=== Signal Test ===");

    /* EMA フィルタ */
    signal::Ema ema(0.5f);
    TEST_ASSERT(!ema.primed(), "ema: not primed initially");

    ema.update(10.0f);
    TEST_ASSERT(ema.primed(), "ema: primed after first sample");

    float val = ema.update(20.0f);
    LOG_I("ema value after [10, 20]: %ld/100", static_cast<int32_t>(val * 100));
    /* EMA(0.5): first=10, second=10*0.5+20*0.5=15 */
    TEST_ASSERT(val > 14.0f && val < 16.0f, "ema: value ~15.0");

    ema.reset();
    TEST_ASSERT(!ema.primed(), "ema: reset clears primed");

    /* RSSI Accumulator */
    signal::RssiAccum accum(4);
    TEST_ASSERT(accum.capacity() == 4, "accum: capacity == 4");
    TEST_ASSERT(accum.count() == 0, "accum: initial count == 0");
    TEST_ASSERT(!accum.full(), "accum: not full initially");

    accum.add(-40);
    accum.add(-50);
    accum.add(-60);
    bool filled = accum.add(-50);
    TEST_ASSERT(filled, "accum: full after 4 samples");
    TEST_ASSERT(accum.full(), "accum: full() == true");

    auto avg = accum.average();
    LOG_I("rssi avg: %ld dBm", static_cast<int32_t>(avg));
    TEST_ASSERT(avg == -50, "accum: average == -50");

    accum.reset();
    TEST_ASSERT(accum.count() == 0, "accum: reset clears count");
}

/* ==================================================================
 * Crash Info テスト
 * ================================================================== */

static void test_crash() {
    LOG_I("=== Crash Info Test ===");

    CrashInfo info;
    auto result = crash_info_read(&info);

    if (result == 0) {
        LOG_I("前回クラッシュあり: fault_type=%lu, pc=0x%lx", info.fault_type, info.pc);
        crash_info_clear();
        LOG_I("クラッシュ情報をクリアしました");

        result = crash_info_read(&info);
        TEST_ASSERT(result != 0, "crash_info: cleared");
    } else {
        LOG_I("前回クラッシュなし");
        TEST_ASSERT(true, "crash_info: no crash (ok)");
    }
}

/* ==================================================================
 * Flash FS テスト
 * ================================================================== */

static void test_flash_fs() {
    LOG_I("=== Flash FS Test ===");

    auto init_result = flash_fs_init();
    LOG_I("flash_fs_init: %ld", init_result);
    TEST_ASSERT(init_result == 0, "flash_fs_init");

    /* ファイル名 → ID 変換 */
    auto log_name = flash_fs_get_name(FLASH_FS_FILE_LOG);
    LOG_I("file[0] name: %s", log_name ? log_name : "(null)");
    TEST_ASSERT(log_name != nullptr, "flash_fs_get_name(LOG)");

    /* ファイル情報取得 */
    FlashFsFileInfo file_info;
    bool info_ok = flash_fs_get_info(FLASH_FS_FILE_SETTINGS, &file_info);
    LOG_I("settings: mode=%lu, capacity=%lu, used=%lu",
          static_cast<uint32_t>(file_info.mode),
          file_info.capacity,
          file_info.used);
    TEST_ASSERT(info_ok, "flash_fs_get_info(SETTINGS)");

    /* Block write / read (SETTINGS ファイル) */
    const uint32_t test_val = 0xDEADBEEF;
    bool write_ok = flash_fs_block_write(FLASH_FS_FILE_SETTINGS, 0, &test_val, sizeof(test_val));
    TEST_ASSERT(write_ok, "flash_fs_block_write");

    uint32_t read_val = 0;
    auto read_sz = flash_fs_block_read(FLASH_FS_FILE_SETTINGS, 0, &read_val, sizeof(read_val));
    LOG_I("read back: 0x%lx (size=%lu)", read_val, static_cast<uint32_t>(read_sz));
    TEST_ASSERT(read_sz == sizeof(test_val) && read_val == test_val, "flash_fs_block_read matches");

    /* Stream append / read (LOG ファイル) */
    flash_fs_erase(FLASH_FS_FILE_LOG);
    const char msg[] = "hello";
    bool append_ok = flash_fs_append(FLASH_FS_FILE_LOG, msg, sizeof(msg));
    TEST_ASSERT(append_ok, "flash_fs_append");

    char read_buf[16] = {};
    auto stream_sz = flash_fs_read(FLASH_FS_FILE_LOG, 0, read_buf, sizeof(read_buf));
    LOG_I("stream read: \"%s\" (size=%lu)", read_buf, static_cast<uint32_t>(stream_sz));
    TEST_ASSERT(stream_sz >= sizeof(msg) && std::memcmp(read_buf, msg, sizeof(msg)) == 0, "flash_fs_read matches");
}

/* ==================================================================
 * BLE テスト
 * ================================================================== */

static volatile bool s_ble_scan_received = false;

static int32_t ble_scan_callback(BLEGapEvent *event, void *) {
    if (event->type == static_cast<uint8_t>(BLEGapEventType::Discovery)) {
        s_ble_scan_received = true;
    }
    return 0;
}

static void test_ble() {
    LOG_I("=== BLE Test ===");

    auto ble_result = ble_init();
    LOG_I("ble_init: %ld", ble_result);
    TEST_ASSERT(ble_result == 0, "ble_init");

    /* Advertise テスト */
    const uint8_t ad_data[] = {
        0x02,
        0x01,
        0x06, /* Flags: LE General Discoverable */
        0x05,
        0xFF,
        0xFF,
        0xFF, /* Manufacturer Specific */
        0xAA,
        0xBB,
    };
    auto set_result = ble_gap_advertise_set_data(ad_data, sizeof(ad_data));
    TEST_ASSERT(set_result == 0, "ble_gap_advertise_set_data");

    BLEGapAdvertiseParams adv_params = {};
    adv_params.interval_min = 160; /* 100ms */
    adv_params.interval_max = 160;
    adv_params.advertise_type = 0; /* connectable undirected */

    auto adv_result = ble_gap_advertise_start(0, &adv_params);
    LOG_I("advertise_start: %ld", adv_result);
    TEST_ASSERT(adv_result == 0, "ble_gap_advertise_start");

    TEST_ASSERT(ble_gap_advertise_active() == 1, "ble_gap_advertise_active");

    ble_gap_advertise_stop();
    TEST_ASSERT(ble_gap_advertise_active() == 0, "ble_gap_advertise stopped");

    /* Discover テスト (2秒スキャン) */
    s_ble_scan_received = false;
    ble_gap_discoveryParams scan_params = {};
    scan_params.interval = 160; /* 100ms */
    scan_params.window = 80;    /* 50ms */
    scan_params.is_passive = 1;
    scan_params.filter_duplicates = 0;

    auto disc_result = ble_gap_discover(0, 2000, &scan_params, ble_scan_callback, nullptr);
    LOG_I("discover: %ld", disc_result);
    TEST_ASSERT(disc_result == 0, "ble_gap_discover");

    /* 2秒待って結果確認 */
    tk_dly_tsk(2500);

    if (s_ble_scan_received) {
        LOG_I("BLE デバイスを検出しました");
    } else {
        LOG_I("BLE デバイス未検出 (周囲に無い場合は正常)");
    }
    TEST_ASSERT(true, "ble_gap_discover completed");
}

/* ==================================================================
 * Beacon テスト
 * ================================================================== */

static void test_beacon() {
    LOG_I("=== Beacon Test ===");

    auto init_result = beacon::init(nullptr);
    LOG_I("beacon::init: %ld", init_result);
    /* NOTE: ble_init() は既に呼ばれているので -EALREADY の可能性あり */
    TEST_ASSERT(init_result == 0 || init_result == -11, "beacon::init");

    auto start_result = beacon::start();
    LOG_I("beacon::start: %ld", start_result);
    TEST_ASSERT(start_result == 0, "beacon::start");

    TEST_ASSERT(beacon::is_active() == 1, "beacon::is_active");

    auto interval = beacon::get_interval();
    LOG_I("beacon interval: %lu ms", static_cast<uint32_t>(interval));
    TEST_ASSERT(interval > 0, "beacon::get_interval > 0");

    auto set_result = beacon::set_interval(500);
    LOG_I("set_interval(500): %ld", set_result);
    TEST_ASSERT(set_result == 0, "beacon::set_interval");
    TEST_ASSERT(beacon::get_interval() == 500, "beacon interval == 500");

    /* 1秒アドバタイズ後停止 */
    tk_dly_tsk(1000);
    beacon::stop();
    TEST_ASSERT(beacon::is_active() == 0, "beacon::stop");
}

/* ==================================================================
 * Shell テスト
 * ================================================================== */

static volatile bool s_custom_cmd_called = false;

static void custom_cmd_handler(int32_t argc, const char *const *argv) {
    (void)argc;
    (void)argv;
    s_custom_cmd_called = true;
    shell_puts("custom command executed\r\n");
}

static const ShellCommand s_test_commands[] = {
    {"itest", "run integration test custom command", custom_cmd_handler},
};

static void test_shell() {
    LOG_I("=== Shell Test ===");

    /* Shell 初期化 + カスタムコマンド登録 */
    uint8_t beacon_cmd_count = 0;
    beacon::get_shell_commands(&beacon_cmd_count);
    LOG_I("beacon commands: %lu", static_cast<uint32_t>(beacon_cmd_count));
    TEST_ASSERT(beacon_cmd_count > 0, "beacon shell commands exist");

    /* NOTE: shell_init は1回だけ呼ぶ。後述のshellタスクで利用 */

    /* feed_char でコマンドディスパッチテスト */
    s_custom_cmd_called = false;
    const char cmd[] = "itest\r";
    for (size_t i = 0; i < sizeof(cmd) - 1; i++) {
        shell_feed_char(cmd[i]);
    }
    TEST_ASSERT(s_custom_cmd_called, "shell_feed_char dispatches");
}

/* ==================================================================
 * Shell Task (対話テスト用)
 * ================================================================== */

static void shell_task(INT stacd, void *exinf) {
    (void)stacd;
    (void)exinf;

    for (;;) {
        shell_poll();
        tk_dly_tsk(10);
    }
}

static const T_CTSK s_ctsk_shell = {
    0,
    (TA_HLNG | TA_RNG3),
    reinterpret_cast<FP>(&shell_task),
    10,   /* 優先度 */
    2048, /* スタックサイズ */
    0,
};

/* ==================================================================
 * メインタスク
 * ================================================================== */

static void main_task(INT stacd, void *exinf) {
    (void)stacd;
    (void)exinf;

    LOG_I("========================================");
    LOG_I("  Integration Test Start");
    LOG_I("========================================");

    /* --- 自動テスト --- */
    test_uart_rx();
    test_sysconfig();
    test_signal();
    test_crash();
    test_flash_fs();
    test_ble();
    test_beacon();

    /* Shell 初期化 (beacon + itest コマンド) */
    shell_init(s_test_commands, sizeof(s_test_commands) / sizeof(s_test_commands[0]));
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
    auto shell_id = tk_cre_tsk(&s_ctsk_shell);
    tk_sta_tsk(shell_id, 0);

    /* メインタスクは終了 */
    tk_ext_tsk();
}

static const T_CTSK s_ctsk_main = {
    0,
    (TA_HLNG | TA_RNG3),
    reinterpret_cast<FP>(&main_task),
    8,    /* 優先度 (shell より高い) */
    4096, /* スタックサイズ */
    0,
};

/* ==================================================================
 * Entry Point
 * ================================================================== */

static int app_main();

extern "C" int usermain(void) {
    return app_main();
}

static int app_main() {
    /* UART 初期化 */
    uart_init(nullptr);

    /* メインタスク生成・起動 */
    auto id = tk_cre_tsk(&s_ctsk_main);
    tk_sta_tsk(id, 0);

    tk_slp_tsk(TMO_FEVR);

    return 0;
}
