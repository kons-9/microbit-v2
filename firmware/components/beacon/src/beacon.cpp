/**
 * @file beacon.cpp
 * @brief BLE ビーコン制御モジュール実装
 */

#include "beacon.h"
#include "ble.h"
#include "shell.h"

#define LOG_TAG "BEACON"
#include "log.h"

#include <cstring>
#include <cstdlib>

namespace beacon {

/* ================================================================== */
/*  Constants                                                         */
/* ================================================================== */

static constexpr uint16_t DEFAULT_INTERVAL_MS = 1000;
static constexpr int8_t DEFAULT_TX_POWER = 0;
static constexpr uint8_t DEFAULT_COMPANY_ID_LO = 0xFF;
static constexpr uint8_t DEFAULT_COMPANY_ID_HI = 0xFF;

static constexpr uint16_t INTERVAL_MIN_MS = 20;
static constexpr uint16_t INTERVAL_MAX_MS = 10240;

/* BLE 仕様: interval 単位は 0.625ms */
static constexpr uint16_t MS_TO_BLE_UNITS(uint16_t ms) {
    return static_cast<uint16_t>((ms * 1000UL) / 625);
}

/* ================================================================== */
/*  State                                                             */
/* ================================================================== */

static Config s_config = {};
static int32_t s_initialized = 0;

/* ================================================================== */
/*  AD Data builder                                                   */
/* ================================================================== */

static uint8_t s_adData[BLE_ADVERTISE_DATA_MAX_LENGTH];
static uint8_t s_adDataLength = 0;

static void build_ad_data(void) {
    uint8_t pos = 0;

    /* --- Flags --- */
    s_adData[pos++] = 0x02;
    s_adData[pos++] = 0x01;
    s_adData[pos++] = 0x06;

    /* --- Manufacturer Specific Data --- */
    uint8_t mfr_start = pos;
    pos++;
    s_adData[pos++] = 0xFF;
    s_adData[pos++] = s_config.company_id_lo;
    s_adData[pos++] = s_config.company_id_hi;
    s_adData[pos++] = static_cast<uint8_t>(s_config.tx_power);

    s_adData[mfr_start] = static_cast<uint8_t>(pos - mfr_start - 1);

    /* --- TX Power Level --- */
    s_adData[pos++] = 0x02;
    s_adData[pos++] = 0x0A;
    s_adData[pos++] = static_cast<uint8_t>(s_config.tx_power);

    s_adDataLength = pos;
}

/* ================================================================== */
/*  Internal helpers                                                  */
/* ================================================================== */

static int32_t start_advertising(void) {
    build_ad_data();

    auto result = ble_gap_advertise_set_data(s_adData, s_adDataLength);
    if (result != 0) {
        return result;
    }

    BLEGapAdvertiseParams params = {};
    params.interval_min = MS_TO_BLE_UNITS(s_config.interval_ms);
    params.interval_max = MS_TO_BLE_UNITS(s_config.interval_ms);
    params.advertise_type = 2;

    return ble_gap_advertise_start(static_cast<uint8_t>(BLEAddressType::Random), &params);
}

/* ================================================================== */
/*  Shell command handler                                             */
/* ================================================================== */

static void cmd_beacon(int32_t argc, const char *const *argv) {
    if (argc < 2) {
        shell_puts("Usage: beacon <start|stop|status|interval [ms]>\r\n");
        return;
    }

    if (std::strcmp(argv[1], "start") == 0) {
        if (is_active()) {
            shell_puts("Already active\r\n");
            return;
        }
        auto result = start();
        if (result == 0) {
            shell_puts("OK\r\n");
        } else {
            shell_printf("Error: %ld\r\n", static_cast<long>(result));
        }
    } else if (std::strcmp(argv[1], "stop") == 0) {
        stop();
        shell_puts("OK\r\n");
    } else if (std::strcmp(argv[1], "status") == 0) {
        if (is_active()) {
            shell_printf("Beacon: active (interval=%u ms)\r\n", static_cast<unsigned>(s_config.interval_ms));
        } else {
            shell_puts("Beacon: stopped\r\n");
        }
    } else if (std::strcmp(argv[1], "interval") == 0) {
        if (argc < 3) {
            shell_printf("interval: %u ms\r\n", static_cast<unsigned>(s_config.interval_ms));
            return;
        }
        auto val = static_cast<uint16_t>(std::strtoul(argv[2], nullptr, 10));
        auto result = set_interval(val);
        if (result == 0) {
            shell_printf("OK: interval=%u ms\r\n", static_cast<unsigned>(val));
        } else {
            shell_puts("Error: invalid interval (20-10240 ms)\r\n");
        }
    } else {
        shell_puts("Unknown subcommand\r\n");
    }
}

static const ShellCommand BEACON_CMDS[] = {
    {"beacon", "beacon <start|stop|status|interval> - BLE beacon control", cmd_beacon},
};

static constexpr uint8_t BEACON_CMD_COUNT = static_cast<uint8_t>(sizeof(BEACON_CMDS) / sizeof(BEACON_CMDS[0]));

/* ================================================================== */
/*  Public API                                                        */
/* ================================================================== */

int32_t init(const Config *config) {
    if (config != nullptr) {
        s_config = *config;
    } else {
        s_config.interval_ms = DEFAULT_INTERVAL_MS;
        s_config.tx_power = DEFAULT_TX_POWER;
        s_config.company_id_lo = DEFAULT_COMPANY_ID_LO;
        s_config.company_id_hi = DEFAULT_COMPANY_ID_HI;
    }

    s_initialized = 1;
    LOG_D("init: interval=%u ms, tx_power=%d", s_config.interval_ms, s_config.tx_power);
    return ble_init();
}

int32_t start(void) {
    if (!s_initialized) {
        return -1;
    }
    if (is_active()) {
        return static_cast<int32_t>(BLEError::Busy);
    }
    LOG_D("start advertising");
    return start_advertising();
}

int32_t stop(void) {
    LOG_D("stop");
    return ble_gap_advertise_stop();
}

int32_t is_active(void) {
    return ble_gap_advertise_active();
}

int32_t set_interval(uint16_t interval_ms) {
    if (interval_ms < INTERVAL_MIN_MS || interval_ms > INTERVAL_MAX_MS) {
        return -1;
    }

    s_config.interval_ms = interval_ms;

    /* 発信中なら再起動 */
    if (is_active()) {
        ble_gap_advertise_stop();
        return start_advertising();
    }

    return 0;
}

uint16_t get_interval(void) {
    return s_config.interval_ms;
}

const ShellCommand *get_shell_commands(uint8_t *count) {
    if (count != nullptr) {
        *count = BEACON_CMD_COUNT;
    }
    return BEACON_CMDS;
}

}  // namespace beacon
