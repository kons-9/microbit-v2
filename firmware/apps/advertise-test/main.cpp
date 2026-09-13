/**
 * @file main.cpp
 * @brief RADIO直叩きAdvertisingの単体確認アプリ
 *
 * 起動するとBLE_TESTという名前でAdvertisingを開始する。
 */

#define LOG_TAG "ADV_TEST"
#include "log.h"

#include <utkernel/task>

#include "ble.h"
#include "uart.h"

#include <cstdint>

static drivers::Uart s_uart;

static constexpr uint16_t ADVERTISING_INTERVAL_MS = 100;

/* Flags + Complete Local Name(BLE_TEST) */
static constexpr uint8_t ADVERTISE_DATA[] = {
    0x02,
    0x01,
    0x06,
    0x09,
    0x09,
    'B',
    'L',
    'E',
    '_',
    'T',
    'E',
    'S',
    'T',
};

static void log_debug_status(void) {
    ble::AdvertiseDebugStatus status = {};
    if (ble::gap_advertise_get_debug_status(&status) != 0) {
        LOG_E("failed to get advertising debug status");
        return;
    }

    LOG_I("adv dbg timer=%lu events=%lu attempts=%lu complete=%lu timeout=%lu error=%lu pdu_len=%lu timer_cc=%lu",
          static_cast<unsigned long>(status.timer_interrupt_count),
          static_cast<unsigned long>(status.advertise_event_count),
          static_cast<unsigned long>(status.tx_attempt_count),
          static_cast<unsigned long>(status.tx_complete_count),
          static_cast<unsigned long>(status.tx_timeout_count),
          static_cast<unsigned long>(status.tx_error_count),
          static_cast<unsigned long>(status.pdu_length),
          static_cast<unsigned long>(status.timer_compare));
    LOG_I("adv dbg last ch=%lu freq=%lu state=%lx events=%lx crc=%lu",
          static_cast<unsigned long>(status.last_channel),
          static_cast<unsigned long>(status.last_frequency),
          static_cast<unsigned long>(status.last_radio_state),
          static_cast<unsigned long>(status.last_radio_events),
          static_cast<unsigned long>(status.last_crcstatus));
    LOG_I("radio cfg mode=%lx pcnf0=%lx pcnf1=%lx base0=%lx prefix0=%lx",
          static_cast<unsigned long>(status.radio_mode),
          static_cast<unsigned long>(status.radio_pcnf0),
          static_cast<unsigned long>(status.radio_pcnf1),
          static_cast<unsigned long>(status.radio_base0),
          static_cast<unsigned long>(status.radio_prefix0));
    LOG_I("radio cfg crccnf=%lx crcpoly=%lx crcinit=%lx txpower=%lx tifs=%lu shorts=%lx",
          static_cast<unsigned long>(status.radio_crccnf),
          static_cast<unsigned long>(status.radio_crcpoly),
          static_cast<unsigned long>(status.radio_crcinit),
          static_cast<unsigned long>(status.radio_txpower),
          static_cast<unsigned long>(status.radio_tifs),
          static_cast<unsigned long>(status.radio_shorts));
    LOG_I("radio cfg packetptr=%lx txaddress=%lx whiteiv=%lx",
          static_cast<unsigned long>(status.radio_packetptr),
          static_cast<unsigned long>(status.radio_txaddress),
          static_cast<unsigned long>(status.radio_datawhiteiv));
    LOG_I("radio live power=%lx freq=%lu rxaddresses=%lx mismatch=%lx",
          static_cast<unsigned long>(status.radio_power),
          static_cast<unsigned long>(status.radio_frequency),
          static_cast<unsigned long>(status.radio_rxaddresses),
          static_cast<unsigned long>(status.radio_config_mismatch));
    LOG_I("ficr part=%lx variant=%lx package=%lx ram=%lx flash=%lx deviceid=%lx:%lx",
          static_cast<unsigned long>(status.ficr_part),
          static_cast<unsigned long>(status.ficr_variant),
          static_cast<unsigned long>(status.ficr_package),
          static_cast<unsigned long>(status.ficr_ram),
          static_cast<unsigned long>(status.ficr_flash),
          static_cast<unsigned long>(status.ficr_deviceid1),
          static_cast<unsigned long>(status.ficr_deviceid0));
}

static utkernel::task s_status_task;

static void status_task(void *) {

    for (;;) {
        LOG_I("advertising=%s interval=%u ms",
              ble::gap_advertise_active() ? "on" : "off",
              static_cast<unsigned>(ADVERTISING_INTERVAL_MS));
        log_debug_status();
        utkernel::task::sleep_for(1000);
    }
}

extern "C" int usermain(void) {
    s_uart.init();
    logging::Logger::instance().init(logging::LogLevel::Debug, s_uart);

    auto result = ble::init();
    if (result != 0) {
        LOG_E("ble init failed: %ld", static_cast<long>(result));
        utkernel::task::sleep_forever();
        return -1;
    }

    result = ble::gap_advertise_set_data(ADVERTISE_DATA, sizeof(ADVERTISE_DATA));
    if (result != 0) {
        LOG_E("set advertising data failed: %ld", static_cast<long>(result));
        utkernel::task::sleep_forever();
        return -1;
    }

    ble::GapAdvertiseParams params = {};
    params.interval_min = static_cast<uint16_t>((ADVERTISING_INTERVAL_MS * 1000UL) / 625UL);
    params.interval_max = params.interval_min;
    /* 切り分け用に接続可能広告を使用。PDUがBLE Explorerに見えるか確認する。 */
    params.advertise_type = ble::AdvertisePduType::ConnectableUndirected;

    result = ble::gap_advertise_start(ble::AddressType::Random, &params);
    if (result != 0) {
        LOG_E("advertising start failed: %ld", static_cast<long>(result));
        utkernel::task::sleep_forever();
        return -1;
    }

    LOG_I("advertising started: BLE_TEST, interval=%u ms", static_cast<unsigned>(ADVERTISING_INTERVAL_MS));
    logging::Logger::instance().hex_dump(logging::LogLevel::Info, LOG_TAG, ADVERTISE_DATA, sizeof(ADVERTISE_DATA));

    utkernel::task::config task_config;
    task_config.priority = 10;
    task_config.stack_size = 1024;
    if (!s_status_task.create(status_task, task_config) || !s_status_task.start()) {
        LOG_E("status task create/start failed");
    }

    utkernel::task::sleep_forever();
    return 0;
}
