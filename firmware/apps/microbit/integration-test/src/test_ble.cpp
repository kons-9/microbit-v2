/**
 * @file test_ble.cpp
 * @brief BLE統合テスト
 */

#define LOG_TAG "ITEST"
#include "integration_test.h"

#include "ble.h"
#include "log.h"

#include <utkernel/task>

namespace integration_test {

namespace {

volatile bool s_ble_scan_received = false;

int32_t ble_scan_callback(ble::GapEvent *event, void *) {
    if (event->type == ble::GapEventType::Discovery) {
        s_ble_scan_received = true;
    }
    return 0;
}

}  // namespace

void run_ble_test(Context &context) {
    LOG_I("=== BLE Test ===");

    auto ble_result = ble::init();
    LOG_I("ble_init: %ld", ble_result);
    context.assert_true(ble_result == 0, "ble_init");

    const uint8_t ad_data[] = {
        0x02, 0x01, 0x06, 0x11, 0x09, 'i', 'n', 't', 'e', 'g', 'r', 'a', 't', 'i', 'o', 'n', ' ', 't', 'e', 's', 't',
    };
    auto set_result = ble::gap_advertise_set_data(ad_data, sizeof(ad_data));
    context.assert_true(set_result == 0, "ble::gap_advertise_set_data");

    ble::GapAdvertiseParams adv_params = {};
    adv_params.interval_min = 160;
    adv_params.interval_max = 160;
    adv_params.advertise_type = ble::AdvertisePduType::ConnectableUndirected;

    auto adv_result = ble::gap_advertise_start(ble::AddressType::Public, &adv_params);
    LOG_I("advertise_start: %ld", adv_result);
    context.assert_true(adv_result == 0, "ble::gap_advertise_start");
    context.assert_true(ble::gap_advertise_active() == 1, "ble::gap_advertise_active");

    ble::gap_advertise_stop();
    context.assert_true(ble::gap_advertise_active() == 0, "ble_gap_advertise stopped");

    s_ble_scan_received = false;
    ble::DiscoveryParams scan_params = {};
    scan_params.interval = 160;
    scan_params.window = 80;
    scan_params.is_passive = 1;
    scan_params.filter_duplicates = 0;

    auto disc_result = ble::gap_discover(ble::AddressType::Public, 2000, &scan_params, ble_scan_callback, nullptr);
    LOG_I("discover: %ld", disc_result);
    context.assert_true(disc_result == 0, "ble_gap_discover");

    utkernel::task::sleep_for(2500);

    LOG_I(s_ble_scan_received ? "BLE device found" : "no BLE device found (ok if none nearby)");
    context.assert_true(true, "ble_gap_discover completed");
}

}  // namespace integration_test
