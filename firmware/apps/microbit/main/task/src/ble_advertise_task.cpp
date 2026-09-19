/**
 * @file ble_advertise_task.cpp
 * @brief メインアプリケーションのBLE Advertisingタスク実装
 */

#define LOG_TAG "BLE_TASK"
#include "ble_advertise_task.h"

#include "ble.h"
#include "log.h"

#include <cstdint>

namespace app::task {

namespace {

static constexpr uint16_t ADVERTISING_INTERVAL_MS = 100;

/* Flags + Complete Local Name (BLE_LOCATOR) */
static constexpr uint8_t ADVERTISE_DATA[] = {
    0x02,
    0x01,
    0x06,
    0x0C,
    0x09,
    'B',
    'L',
    'E',
    '_',
    'L',
    'O',
    'C',
    'A',
    'T',
    'O',
    'R',
};

static void log_debug_status(drivers::Ble &ble_driver) {
    ble::AdvertiseDebugStatus status = {};
    if (ble_driver.gap_advertise_get_debug_status(&status) != 0) {
        LOG_E("failed to get advertising debug status");
        return;
    }

    LOG_D("advertising=%s interval=%u ms",
          ble_driver.gap_advertise_active() ? "on" : "off",
          static_cast<unsigned>(ADVERTISING_INTERVAL_MS));
    LOG_D("adv dbg timer=%lu events=%lu attempts=%lu complete=%lu timeout=%lu error=%lu pdu_len=%lu timer_cc=%lu",
          static_cast<unsigned long>(status.timer_interrupt_count),
          static_cast<unsigned long>(status.advertise_event_count),
          static_cast<unsigned long>(status.tx_attempt_count),
          static_cast<unsigned long>(status.tx_complete_count),
          static_cast<unsigned long>(status.tx_timeout_count),
          static_cast<unsigned long>(status.tx_error_count),
          static_cast<unsigned long>(status.pdu_length),
          static_cast<unsigned long>(status.timer_compare));
    LOG_D("adv dbg last ch=%lu freq=%lu state=%lx events=%lx crc=%lu",
          static_cast<unsigned long>(status.last_channel),
          static_cast<unsigned long>(status.last_frequency),
          static_cast<unsigned long>(status.last_radio_state),
          static_cast<unsigned long>(status.last_radio_events),
          static_cast<unsigned long>(status.last_crcstatus));
    LOG_D("radio cfg mode=%lx pcnf0=%lx pcnf1=%lx base0=%lx prefix0=%lx",
          static_cast<unsigned long>(status.radio_mode),
          static_cast<unsigned long>(status.radio_pcnf0),
          static_cast<unsigned long>(status.radio_pcnf1),
          static_cast<unsigned long>(status.radio_base0),
          static_cast<unsigned long>(status.radio_prefix0));
    LOG_D("radio cfg crccnf=%lx crcpoly=%lx crcinit=%lx txpower=%lx tifs=%lu shorts=%lx",
          static_cast<unsigned long>(status.radio_crccnf),
          static_cast<unsigned long>(status.radio_crcpoly),
          static_cast<unsigned long>(status.radio_crcinit),
          static_cast<unsigned long>(status.radio_txpower),
          static_cast<unsigned long>(status.radio_tifs),
          static_cast<unsigned long>(status.radio_shorts));
    LOG_D("radio cfg packetptr=%lx txaddress=%lx whiteiv=%lx",
          static_cast<unsigned long>(status.radio_packetptr),
          static_cast<unsigned long>(status.radio_txaddress),
          static_cast<unsigned long>(status.radio_datawhiteiv));
    LOG_D("radio live power=%lx freq=%lu rxaddresses=%lx mismatch=%lx",
          static_cast<unsigned long>(status.radio_power),
          static_cast<unsigned long>(status.radio_frequency),
          static_cast<unsigned long>(status.radio_rxaddresses),
          static_cast<unsigned long>(status.radio_config_mismatch));
    LOG_D("ficr part=%lx variant=%lx package=%lx ram=%lx flash=%lx deviceid=%lx:%lx",
          static_cast<unsigned long>(status.ficr_part),
          static_cast<unsigned long>(status.ficr_variant),
          static_cast<unsigned long>(status.ficr_package),
          static_cast<unsigned long>(status.ficr_ram),
          static_cast<unsigned long>(status.ficr_flash),
          static_cast<unsigned long>(status.ficr_deviceid1),
          static_cast<unsigned long>(status.ficr_deviceid0));
}

}  // namespace

BleAdvertiseTask BleAdvertiseTask::s_instance;

BleAdvertiseTask &BleAdvertiseTask::instance() {
    return s_instance;
}

bool BleAdvertiseTask::start() {
    if (m_task.joinable()) {
        return false;
    }

    utkernel::task::config task_config;
    task_config.name = "ble_advertise";
    task_config.priority = 10;
    task_config.stack_size = 2048;
    task_config.param = this;

    if (!m_task.create(entry, task_config)) {
        return false;
    }
    if (!m_task.start()) {
        m_task.terminate();
        return false;
    }
    return true;
}

void BleAdvertiseTask::entry(void *argument) {
    static_cast<BleAdvertiseTask *>(argument)->run();
}

void BleAdvertiseTask::run() {
    auto result = m_ble.init();
    if (result != 0) {
        LOG_E("ble init failed: %ld", static_cast<long>(result));
        utkernel::task::sleep_forever();
        return;
    }

    result = m_ble.gap_advertise_set_data(ADVERTISE_DATA, sizeof(ADVERTISE_DATA));
    if (result != 0) {
        LOG_E("set advertising data failed: %ld", static_cast<long>(result));
        utkernel::task::sleep_forever();
        return;
    }

    ble::GapAdvertiseParams params = {};
    params.interval_min = static_cast<uint16_t>((ADVERTISING_INTERVAL_MS * 1000UL) / 625UL);
    params.interval_max = params.interval_min;
    params.advertise_type = ble::AdvertisePduType::ConnectableUndirected;

    result = m_ble.gap_advertise_start(ble::AddressType::Random, &params);
    if (result != 0) {
        LOG_E("advertising start failed: %ld", static_cast<long>(result));
        utkernel::task::sleep_forever();
        return;
    }

    LOG_I("advertising started: BLE_LOCATOR, interval=%u ms", static_cast<unsigned>(ADVERTISING_INTERVAL_MS));
    logging::Logger::instance().hex_dump(logging::LogLevel::Info, LOG_TAG, ADVERTISE_DATA, sizeof(ADVERTISE_DATA));

    for (;;) {
        log_debug_status(m_ble);
        utkernel::task::sleep_for(1000);
    }
}

}  // namespace app::task
