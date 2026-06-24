/**
 * @file main.cpp
 * @brief BLE 検証用アプリケーション
 *
 * BLE コンポーネントの Advertise / Scan 機能をシェルコマンドで
 * 対話的に検証するためのアプリケーション。
 *
 * シェルコマンド:
 *   adv start [interval_ms]  - アドバタイズ開始 (デフォルト 100ms)
 *   adv stop                 - アドバタイズ停止
 *   adv status               - アドバタイズ状態表示
 *   scan start [duration_ms] - スキャン開始 (デフォルト 5000ms)
 *   scan stop                - スキャン停止
 *   scan status              - スキャン状態表示
 *
 * タスク構成:
 *   - Shell タスク: UART shell ポーリング + スキャン結果ログ出力
 *
 * NOTE: BLE スキャンコールバックは RADIO ISR コンテキストで呼ばれるため、
 *       ISR 内では LOG_* を呼ばず、リングバッファに結果を積む。
 *       タスクコンテキスト (shell_task) でバッファを drain してログ出力する。
 */

#define LOG_TAG "BLE_TEST"
#include "log.h"

#include <tk/tkernel.h>
#include <tm/tmonitor.h>

#include "ble.h"
#include "crash_info.h"
#include "shell.h"
#include "uart.h"

#include <cstdint>
#include <cstdlib>
#include <cstring>

/* ==================================================================
 * Peripheral instances
 * ================================================================== */

static drivers::Uart s_uart;

/* ==================================================================
 * ヘルパー
 * ================================================================== */

#if USE_TMONITOR
#define TM_PUT(a) tm_putstring(a)
#else
#define TM_PUT(a)
#endif

/* ==================================================================
 * Scan result ring buffer (ISR → task)
 *
 * ISR コンテキストでは LOG_* が使えないため、スキャン結果を
 * リングバッファに積み、タスクコンテキストで読み出してログ出力する。
 * ================================================================== */

struct ScanEntry {
    uint8_t address[6];
    int8_t rssi;
    int8_t event_type;
    uint8_t data_length;
};

static constexpr uint8_t SCAN_RING_SIZE = 16;
static volatile ScanEntry s_scan_ring[SCAN_RING_SIZE];
static volatile uint8_t s_scan_ring_write = 0;
static volatile uint8_t s_scan_ring_read = 0;
static volatile uint32_t s_scan_ring_dropped = 0;
static volatile bool s_scan_complete = false;
static volatile int32_t s_scan_complete_reason = 0;

/**
 * ISR コンテキストから呼ばれるスキャンコールバック
 *
 * NOTE: この関数は RADIO 割り込みから呼ばれる可能性がある。
 *       LOG_* やブロッキング API を呼んではならない。
 */
static int32_t scan_event_callback(ble::GapEvent *event, void *argument) {
    (void)argument;

    if (event->type == static_cast<uint8_t>(ble::GapEventType::Discovery)) {
        uint8_t next_write = (s_scan_ring_write + 1) % SCAN_RING_SIZE;
        if (next_write == s_scan_ring_read) {
            /* バッファフル — ドロップ */
            s_scan_ring_dropped = s_scan_ring_dropped + 1;
            return 0;
        }

        auto &entry = const_cast<ScanEntry &>(s_scan_ring[s_scan_ring_write]);
        auto &disc = event->discovery;
        std::memcpy(entry.address, disc.address.value, 6);
        entry.rssi = disc.rssi;
        entry.event_type = disc.event_type;
        entry.data_length = disc.data_length;

        s_scan_ring_write = next_write;
        return 0;
    }

    if (event->type == static_cast<uint8_t>(ble::GapEventType::DiscoveryComplete)) {
        s_scan_complete = true;
        s_scan_complete_reason = event->discovery_complete.reason;
        return 0;
    }

    return 0;
}

/**
 * タスクコンテキストからリングバッファを drain してログ出力する
 */
static void drain_scan_results() {
    while (s_scan_ring_read != s_scan_ring_write) {
        auto &entry = const_cast<ScanEntry &>(s_scan_ring[s_scan_ring_read]);
        LOG_I("[SCAN] addr=%02X:%02X:%02X:%02X:%02X:%02X rssi=%d type=%d len=%d",
              entry.address[5],
              entry.address[4],
              entry.address[3],
              entry.address[2],
              entry.address[1],
              entry.address[0],
              entry.rssi,
              entry.event_type,
              entry.data_length);
        s_scan_ring_read = (s_scan_ring_read + 1) % SCAN_RING_SIZE;
    }

    uint32_t dropped = s_scan_ring_dropped;
    if (dropped > 0) {
        s_scan_ring_dropped = 0;
        LOG_I("[SCAN] dropped %lu entries (buffer full)", static_cast<unsigned long>(dropped));
    }

    if (s_scan_complete) {
        s_scan_complete = false;
        LOG_I("[SCAN] discovery complete (reason=%ld)", static_cast<long>(s_scan_complete_reason));
    }
}

/* ==================================================================
 * Shell command: adv
 * ================================================================== */

static void cmd_adv(int32_t argc, const char *const *argv) {
    if (argc < 2) {
        LOG_I("usage: adv <start|stop|status> [interval_ms]");
        return;
    }

    if (std::strcmp(argv[1], "start") == 0) {
        /* AD データ: Flags(0x06) + 完全ローカル名 "BLE_TEST" */
        static const uint8_t ad_data[] = {
            0x02,
            0x01,
            0x06, /* Flags: LE General + BR/EDR Not Supported */
            0x09,
            0x09,
            'B',
            'L',
            'E',
            '_',
            'T',
            'E',
            'S',
            'T', /* Complete Local Name */
        };

        auto result = ble::gap_advertise_set_data(ad_data, sizeof(ad_data));
        if (result != 0) {
            LOG_E("gap_advertise_set_data failed: %ld", static_cast<long>(result));
            return;
        }

        uint16_t interval_ms = 100;
        if (argc >= 3) {
            interval_ms = static_cast<uint16_t>(std::atoi(argv[2]));
            if (interval_ms < 20) {
                interval_ms = 20;
            }
        }

        /* ms → BLE 単位 (0.625ms) 変換 */
        uint16_t interval_units = static_cast<uint16_t>(interval_ms * 1000 / 625);

        ble::GapAdvertiseParams params = {};
        params.interval_min = interval_units;
        params.interval_max = interval_units;
        params.advertise_type = 2; /* ADV_NONCONN_IND */

        result = ble::gap_advertise_start(static_cast<uint8_t>(ble::AddressType::Random), &params);
        if (result != 0) {
            LOG_E("gap_advertise_start failed: %ld", static_cast<long>(result));
            return;
        }

        LOG_I("advertising started (interval=%u ms)", interval_ms);

    } else if (std::strcmp(argv[1], "stop") == 0) {
        auto result = ble::gap_advertise_stop();
        if (result != 0) {
            LOG_E("gap_advertise_stop failed: %ld", static_cast<long>(result));
            return;
        }
        LOG_I("advertising stopped");

    } else if (std::strcmp(argv[1], "status") == 0) {
        LOG_I("advertising: %s", ble::gap_advertise_active() ? "active" : "inactive");

    } else {
        LOG_I("usage: adv <start|stop|status> [interval_ms]");
    }
}

/* ==================================================================
 * Shell command: scan
 * ================================================================== */

static void cmd_scan(int32_t argc, const char *const *argv) {
    if (argc < 2) {
        LOG_I("usage: scan <start|stop|status> [duration_ms]");
        return;
    }

    if (std::strcmp(argv[1], "start") == 0) {
        int32_t duration_ms = 5000;
        if (argc >= 3) {
            duration_ms = std::atoi(argv[2]);
            if (duration_ms < 0) {
                duration_ms = 0; /* 0 = 無期限 */
            }
        }

        ble::DiscoveryParams params = {};
        params.interval = 160;    /* 100ms (160 * 0.625ms) */
        params.window = 80;       /* 50ms (80 * 0.625ms) */
        params.filter_policy = 0; /* accept all */
        params.is_limited = 0;
        params.is_passive = 1; /* passive scan */
        params.filter_duplicates = 0;

        auto result = ble::gap_discover(static_cast<uint8_t>(ble::AddressType::Random),
                                        duration_ms,
                                        &params,
                                        scan_event_callback,
                                        nullptr);
        if (result != 0) {
            LOG_E("gap_discover failed: %ld", static_cast<long>(result));
            return;
        }

        if (duration_ms == 0) {
            LOG_I("scanning started (duration=unlimited)");
        } else {
            LOG_I("scanning started (duration=%ld ms)", static_cast<long>(duration_ms));
        }

    } else if (std::strcmp(argv[1], "stop") == 0) {
        auto result = ble::gap_discover_cancel();
        if (result != 0) {
            LOG_E("gap_discover_cancel failed: %ld", static_cast<long>(result));
            return;
        }
        LOG_I("scanning stopped");

    } else if (std::strcmp(argv[1], "status") == 0) {
        LOG_I("scanning: %s", ble::gap_discovery_active() ? "active" : "inactive");

    } else {
        LOG_I("usage: scan <start|stop|status> [duration_ms]");
    }
}

/* ==================================================================
 * Shell command: log
 * ================================================================== */

static void cmd_log(int32_t argc, const char *const *argv) {
    if (argc < 2) {
        LOG_I("usage: log <debug|info|error>");
        return;
    }

    if (std::strcmp(argv[1], "debug") == 0) {
        LogSetLevel(LOG_LEVEL_DEBUG);
        LOG_I("log level: DEBUG");
    } else if (std::strcmp(argv[1], "info") == 0) {
        LogSetLevel(LOG_LEVEL_INFO);
        LOG_I("log level: INFO");
    } else if (std::strcmp(argv[1], "error") == 0) {
        LogSetLevel(LOG_LEVEL_ERROR);
        LOG_I("log level: ERROR");
    } else {
        LOG_I("usage: log <debug|info|error>");
    }
}

/* ==================================================================
 * Shell commands table
 * ================================================================== */

static const shell::Command s_commands[] = {
    {"adv", "adv <start|stop|status> [interval_ms] - BLE advertise control", cmd_adv},
    {"scan", "scan <start|stop|status> [duration_ms] - BLE scan control", cmd_scan},
    {"log", "log <debug|info|error> - set log level", cmd_log},
};

static constexpr uint8_t COMMAND_COUNT = static_cast<uint8_t>(sizeof(s_commands) / sizeof(s_commands[0]));

/* ==================================================================
 * Shell Task
 * ================================================================== */

static void shell_task(INT stacd, void *exinf) {
    (void)stacd;
    (void)exinf;

    for (;;) {
        shell::poll();
        drain_scan_results();
        tk_dly_tsk(10);
    }
}

static const T_CTSK s_ctsk_shell = {0, (TA_HLNG | TA_RNG3), reinterpret_cast<FP>(&shell_task), 10, 2048, 0};

/* ==================================================================
 * Entry Point
 * ================================================================== */

static int app_main();

extern "C" int usermain(void) {
    return app_main();
}

static int app_main() {
    TM_PUT(reinterpret_cast<UB *>(const_cast<char *>("BLE Test App\n")));

    /* UART 初期化 */
    s_uart.init();
    LogInit(LOG_LEVEL_INFO, s_uart);

    /* BLE 初期化 */
    auto result = ble::init();
    if (result != 0) {
        LOG_E("ble::init failed: %ld", static_cast<long>(result));
        return -1;
    }
    LOG_I("BLE initialized");

    /* Shell 初期化 */
    shell::init(s_uart, s_commands, COMMAND_COUNT);

    LOG_I("commands: adv, scan, log");
    LOG_I("  adv start [interval_ms]  - start advertising (default 100ms)");
    LOG_I("  adv stop                 - stop advertising");
    LOG_I("  scan start [duration_ms] - start scanning (default 5000ms)");
    LOG_I("  scan stop                - stop scanning");
    LOG_I("  log <debug|info|error>   - set log level");

    /* Shell タスク生成・起動 */
    auto id = tk_cre_tsk(&s_ctsk_shell);
    tk_sta_tsk(id, 0);

    tk_slp_tsk(TMO_FEVR);

    return 0;
}
