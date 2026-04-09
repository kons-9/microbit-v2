/*
 * ble_locator/main.cpp — BLE Indoor Position Estimator
 *
 * 1. Periodically scans BLE beacons for RSSI values
 * 2. Sends RSSI + beacon positions to external AI via μAI-Bridge
 * 3. AI model estimates the person's (x,y) position
 * 4. Tracks and displays position on an ASCII room map
 * 5. Supports ACCURACY / RESPONSIVE estimation mode switching
 *
 * Build & Run:
 *   1. Start host:  python ../../rtos-middleware/host/uai_host.py --port 5000
 *   2. Run app:     ./ble_locator [host] [port]
 */

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "app_config.h"
#include "ble_scanner.h"
#include "ble_service.h"
#include "location.h"
#include "estimation_mode.h"
#include "arch/arch.h"

#include "core/uai_bridge.h"
#include "transport/uai_tcp.h"
#include "arch/uai_task_monitor.h"

/*
 * Pack scan result into a flat buffer for AI inference:
 *   [n_beacons(uint8)] [rssi_0(int8) x_0(f32) y_0(f32)] ...
 */
static int pack_scan_data(const ble::ScanResult &scan,
                          uint8_t *buf, int buf_len)
{
    int needed = 1 + scan.n_beacons * (1 + 4 + 4); /* 9 bytes per beacon */
    if (buf_len < needed) return -1;

    int pos = 0;
    buf[pos++] = static_cast<uint8_t>(scan.n_beacons);
    for (int i = 0; i < scan.n_beacons; i++) {
        buf[pos++] = static_cast<uint8_t>(scan.rssi[i]);
        std::memcpy(buf + pos, &scan.beacon_x[i], sizeof(float)); pos += 4;
        std::memcpy(buf + pos, &scan.beacon_y[i], sizeof(float)); pos += 4;
    }
    return pos;
}

/*
 * Unpack AI response: [x(f32) y(f32) confidence(f32)]
 */
static int unpack_position(const uint8_t *buf, uint32_t len,
                           float &x, float &y, float &confidence)
{
    if (len < 12) return -1;
    std::memcpy(&x, buf, sizeof(float));
    std::memcpy(&y, buf + 4, sizeof(float));
    std::memcpy(&confidence, buf + 8, sizeof(float));
    return 0;
}

int main(int argc, char *argv[])
{
    const char *host = "127.0.0.1";
    int port = 5000;
    int max_scans = 15;

    if (argc >= 2) host = argv[1];
    if (argc >= 3) port = std::atoi(argv[2]);
    if (argc >= 4) max_scans = std::atoi(argv[3]);

    std::printf("=== BLE Indoor Position Estimator ===\n");
    std::printf("Connecting to AI server %s:%d ...\n\n", host, port);

    /* --- Platform init --- */
    if (ble::arch::init() != 0) {
        std::fprintf(stderr, "Platform init failed\n");
        return 1;
    }

    /* --- Transport & Bridge setup --- */
    uai::TcpTransport transport;
    uai::TcpConfig tcp_cfg{host, static_cast<uint16_t>(port)};

    if (transport.init(tcp_cfg) != uai::OK) {
        std::fprintf(stderr, "TCP init failed\n");
        return 1;
    }

    static uint8_t send_buf[4096];
    static uint8_t recv_buf[4096];
    uai::BridgeConfig bridge_cfg{
        ble::INFER_TIMEOUT_MS,
        send_buf, sizeof(send_buf),
        recv_buf, sizeof(recv_buf)
    };
    uai::Bridge<uai::TcpTransport> bridge(transport, bridge_cfg);

    if (bridge.connect() != uai::OK) {
        std::fprintf(stderr, "Bridge connect failed\n");
        return 1;
    }
    std::printf("Connected to AI server.\n\n");

    /* --- Task monitor setup --- */
    static uint8_t mon_buf[512];
    uai::TaskMonitorConfig mon_cfg{1000, mon_buf, sizeof(mon_buf)};
    uai::TaskMonitor<uai::TcpTransport> monitor(transport, mon_cfg);
    monitor.register_task(1, 8, "ble_scan");
    monitor.register_task(2, 10, "ai_locate");
    monitor.register_task(3, 6, "display");

    /* --- BLE GATT service for smartphone --- */
    ble::BleService ble_svc;
    if (ble_svc.init() != 0) {
        std::printf("Warning: GATT service init failed (no smartphone)\n");
    }

    /* --- BLE scanner setup — register known beacons --- */
    ble::BleScanner scanner;
    scanner.add_beacon("Beacon-A", 1.0f, 1.0f);
    scanner.add_beacon("Beacon-B", 9.0f, 1.0f);
    scanner.add_beacon("Beacon-C", 5.0f, 7.0f);
    scanner.add_beacon("Beacon-D", 1.0f, 7.0f);

    ble::LocationTracker tracker;
    ble::ModeManager mode_mgr;

    /* Start in accuracy mode */
    mode_mgr.set_mode(ble::EstimationMode::ACCURACY);

    std::printf("Registered %d beacons. Starting position tracking...\n",
                scanner.beacon_count());
    mode_mgr.print_status();
    std::printf("\n");

    /* Collect beacon positions for map overlay */
    float beacon_xs[ble::MAX_BEACONS];
    float beacon_ys[ble::MAX_BEACONS];
    char  beacon_names[ble::MAX_BEACONS][16];
    for (int i = 0; i < scanner.beacon_count(); i++) {
        beacon_xs[i] = scanner.beacon(i).known_x;
        beacon_ys[i] = scanner.beacon(i).known_y;
        std::strncpy(beacon_names[i], scanner.beacon(i).name, 15);
        beacon_names[i][15] = '\0';
    }

    /* --- Main loop --- */
    static uint8_t infer_input[256];
    static uint8_t infer_output[64];
    uint32_t prev_tick = ble::arch::millis();

    for (int scan = 0; scan < max_scans; scan++) {
        uint32_t tick = ble::arch::millis();
        uint32_t delta_ms = tick - prev_tick;
        prev_tick = tick;

        const auto &mp = mode_mgr.params();
        std::printf("--- Scan #%d (tick=%u ms, mode=%s) ---\n",
                    scan, tick, mp.display_name);

        /* Task: BLE scan (with mode-dependent sample count) */
        monitor.update(1, 1, 0); /* RUNNING */
        ble::ScanResult scan_result{};
        int rc = scanner.scan_filtered(scan_result, mp.samples_per_scan);
        monitor.update(1, 0, 800); /* READY */

        if (rc != 0) {
            std::printf("  BLE scan failed\n");
            ble::arch::sleep_ms(ble::SCAN_INTERVAL_MS);
            continue;
        }

        /* Print RSSI readings */
        std::printf("  RSSI: ");
        for (int i = 0; i < scan_result.n_beacons; i++) {
            std::printf("%s=%d ", scanner.beacon(i).name,
                        scan_result.rssi[i]);
        }
        std::printf("\n");

        /* Pack scan data for inference */
        int packed = pack_scan_data(scan_result, infer_input,
                                    sizeof(infer_input));
        if (packed < 0) {
            std::printf("  Pack failed\n");
            continue;
        }

        /* Task: AI position estimation */
        monitor.update(2, 1, 0); /* RUNNING */
        uint32_t output_len = sizeof(infer_output);
        rc = bridge.infer(ble::MODEL_BLE_LOCATE,
                          infer_input, static_cast<uint32_t>(packed),
                          infer_output, output_len);
        monitor.update(2, 0, 1500); /* READY */

        if (rc != uai::OK) {
            std::printf("  AI inference failed: %d\n", rc);
            ble::arch::sleep_ms(ble::SCAN_INTERVAL_MS);
            continue;
        }

        /* Unpack position */
        float est_x, est_y, confidence;
        if (unpack_position(infer_output, output_len,
                            est_x, est_y, confidence) != 0) {
            std::printf("  Bad response format\n");
            continue;
        }

        /* Task: display */
        monitor.update(3, 1, 0);

        std::printf("  Estimated position: (%.2f, %.2f) conf=%.3f\n",
                    est_x, est_y, confidence);

        /* Record with mode-dependent smoothing */
        tracker.record(est_x, est_y, confidence, tick,
                       mp.smoothing_alpha);
        tracker.update_zone(delta_ms);

        /* Show speed */
        float spd = tracker.speed();
        if (spd > 0.01f)
            std::printf("  Speed: %.2f m/s\n", spd);

        /* Notify smartphone of position via BLE GATT */
        if (ble_svc.is_connected()) {
            ble_svc.notify_position(
                tracker.latest().x, tracker.latest().y, confidence);
        }

        /* Auto-adjust mode based on confidence trend */
        if (mode_mgr.auto_adjust(confidence)) {
            ble_svc.notify_mode(
                static_cast<uint8_t>(mode_mgr.mode()));
        }

        /* Show position map (every 3rd scan to reduce output) */
        if (scan % 3 == 2 || scan == max_scans - 1) {
            std::printf("\n");
            tracker.print_map_with_beacons(
                beacon_xs, beacon_ys, beacon_names,
                scanner.beacon_count());
            std::printf("\n");
            tracker.print_history(5);
        }

        /* Show zone dwell time at end */
        if (scan == max_scans - 1) {
            std::printf("\n");
            tracker.print_zones();
        }

        monitor.update(3, 0, 100);
        monitor.send_report();

        std::printf("\n");
        ble::arch::sleep_ms(mp.scan_interval_ms);
    }

    std::printf("=== BLE Locator demo finished ===\n");
    bridge.disconnect();
    return 0;
}
