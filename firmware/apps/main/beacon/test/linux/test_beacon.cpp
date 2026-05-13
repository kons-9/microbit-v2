/**
 * @file test_beacon.cpp
 * @brief beacon モジュールのユニットテスト
 */

#include <catch2/catch_test_macros.hpp>
#include "beacon.h"
#include "ble.h"
#include "shell.h"
#include "flash_fs.h"

#include <cstring>

/* ================================================================== */
/*  Mock: UART                                                        */
/* ================================================================== */

extern "C" void mock_uart_reset();
extern "C" const char *mock_uart_get_output();
extern "C" size_t mock_uart_get_output_len();

namespace io { class Stream; }
extern io::Stream &mock_get_stream();

/* ================================================================== */
/*  Mock: BLE (テスト用ステート)                                      */
/* ================================================================== */

extern "C" int32_t mock_ble_get_advertising();
extern "C" const uint8_t *mock_ble_get_adv_data();
extern "C" uint8_t mock_ble_get_adv_data_length();
extern "C" uint16_t mock_ble_get_adv_interval();
extern "C" void mock_ble_reset();

/* ================================================================== */
/*  Flash FS (テスト用)                                               */
/* ================================================================== */

extern "C" void flash_fs_arch_test_reset();

/* ================================================================== */
/*  Fixture                                                           */
/* ================================================================== */

struct BeaconFixture {
    BeaconFixture() {
        flash_fs_arch_test_reset();
        flash_fs::init();
        mock_uart_reset();
        mock_ble_reset();
    }

    void feed_line(const char *line) {
        mock_uart_reset();
        for (const char *p = line; *p; ++p) {
            shell::feed_char(*p);
        }
        shell::feed_char('\r');
    }
};

/* ================================================================== */
/*  beacon::init                                                      */
/* ================================================================== */

TEST_CASE_METHOD(BeaconFixture, "beacon_init with default config succeeds", "[beacon]") {
    REQUIRE(beacon::init(nullptr) == 0);
    REQUIRE(beacon::is_active() == 0);
}

TEST_CASE_METHOD(BeaconFixture, "beacon_init with custom config", "[beacon]") {
    beacon::Config cfg = {500, -4, 0xFF, 0xFF};
    REQUIRE(beacon::init(&cfg) == 0);
    REQUIRE(beacon::get_interval() == 500);
}

/* ================================================================== */
/*  beacon::start / beacon::stop                                      */
/* ================================================================== */

TEST_CASE_METHOD(BeaconFixture, "beacon_start begins advertising", "[beacon]") {
    beacon::init(nullptr);
    REQUIRE(beacon::start() == 0);
    REQUIRE(beacon::is_active() == 1);
    REQUIRE(mock_ble_get_advertising() == 1);
}

TEST_CASE_METHOD(BeaconFixture, "beacon_stop stops advertising", "[beacon]") {
    beacon::init(nullptr);
    beacon::start();
    REQUIRE(beacon::stop() == 0);
    REQUIRE(beacon::is_active() == 0);
    REQUIRE(mock_ble_get_advertising() == 0);
}

TEST_CASE_METHOD(BeaconFixture, "beacon_start sets AD data", "[beacon]") {
    beacon::init(nullptr);
    beacon::start();

    const uint8_t *data = mock_ble_get_adv_data();
    uint8_t len = mock_ble_get_adv_data_length();

    // 最小限: Flags (3B) + Manufacturer Specific Data が含まれるはず
    REQUIRE(len >= 3);
    // AD Structure: Length=2, Type=0x01 (Flags), Data=0x06 (LE General + BR/EDR Not Supported)
    REQUIRE(data[0] == 0x02);  // Length
    REQUIRE(data[1] == 0x01);  // AD Type: Flags
    REQUIRE(data[2] == 0x06);  // Flags value
}

TEST_CASE_METHOD(BeaconFixture, "beacon_start twice returns busy", "[beacon]") {
    beacon::init(nullptr);
    beacon::start();
    REQUIRE(beacon::start() != 0);
}

/* ================================================================== */
/*  beacon::set_interval                                              */
/* ================================================================== */

TEST_CASE_METHOD(BeaconFixture, "beacon_set_interval changes interval", "[beacon]") {
    beacon::init(nullptr);
    REQUIRE(beacon::set_interval(500) == 0);
    REQUIRE(beacon::get_interval() == 500);
}

TEST_CASE_METHOD(BeaconFixture, "beacon_set_interval rejects out of range", "[beacon]") {
    beacon::init(nullptr);
    REQUIRE(beacon::set_interval(10) != 0);     // too small
    REQUIRE(beacon::set_interval(20000) != 0);  // too large
}

TEST_CASE_METHOD(BeaconFixture, "beacon_set_interval restarts if active", "[beacon]") {
    beacon::init(nullptr);
    beacon::start();
    REQUIRE(beacon::set_interval(200) == 0);
    REQUIRE(beacon::is_active() == 1);
    REQUIRE(beacon::get_interval() == 200);
}

/* ================================================================== */
/*  Shell commands                                                    */
/* ================================================================== */

TEST_CASE_METHOD(BeaconFixture, "shell: beacon start/stop", "[beacon][shell]") {
    uint8_t count = 0;
    const shell::Command *cmds = beacon::get_shell_commands(&count);
    REQUIRE(count > 0);

    beacon::init(nullptr);
    shell::init(mock_get_stream(), cmds, count);
    mock_uart_reset();

    feed_line("beacon start");
    REQUIRE(beacon::is_active() == 1);
    const char *out = mock_uart_get_output();
    REQUIRE(std::strstr(out, "OK") != nullptr);

    feed_line("beacon stop");
    REQUIRE(beacon::is_active() == 0);
    out = mock_uart_get_output();
    REQUIRE(std::strstr(out, "OK") != nullptr);
}

TEST_CASE_METHOD(BeaconFixture, "shell: beacon status", "[beacon][shell]") {
    uint8_t count = 0;
    const shell::Command *cmds = beacon::get_shell_commands(&count);
    beacon::init(nullptr);
    shell::init(mock_get_stream(), cmds, count);
    mock_uart_reset();

    feed_line("beacon status");
    const char *out = mock_uart_get_output();
    REQUIRE(std::strstr(out, "stopped") != nullptr);

    beacon::start();
    feed_line("beacon status");
    out = mock_uart_get_output();
    REQUIRE(std::strstr(out, "active") != nullptr);
}

TEST_CASE_METHOD(BeaconFixture, "shell: beacon interval", "[beacon][shell]") {
    uint8_t count = 0;
    const shell::Command *cmds = beacon::get_shell_commands(&count);
    beacon::init(nullptr);
    shell::init(mock_get_stream(), cmds, count);
    mock_uart_reset();

    feed_line("beacon interval 500");
    const char *out = mock_uart_get_output();
    REQUIRE(std::strstr(out, "500") != nullptr);
    REQUIRE(beacon::get_interval() == 500);
}

TEST_CASE_METHOD(BeaconFixture, "shell: help includes beacon", "[beacon][shell]") {
    uint8_t count = 0;
    const shell::Command *cmds = beacon::get_shell_commands(&count);
    beacon::init(nullptr);
    shell::init(mock_get_stream(), cmds, count);
    mock_uart_reset();

    feed_line("help");
    const char *out = mock_uart_get_output();
    REQUIRE(std::strstr(out, "beacon") != nullptr);
}
