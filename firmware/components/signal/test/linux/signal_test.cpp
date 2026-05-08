#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "signal_proc.h"

using Catch::Matchers::WithinAbs;

TEST_CASE("EMA basic smoothing", "[signal][ema]") {
    SignalEma ema;
    signal_ema_init(&ema, 0.3f);

    float v = signal_ema_update(&ema, -60.0f);
    REQUIRE_THAT(v, WithinAbs(-60.0f, 0.01f));

    v = signal_ema_update(&ema, -50.0f);
    // expected: 0.3*(-50) + 0.7*(-60) = -57
    REQUIRE_THAT(v, WithinAbs(-57.0f, 0.01f));
}

TEST_CASE("EMA alpha=1 means no smoothing", "[signal][ema]") {
    SignalEma ema;
    signal_ema_init(&ema, 1.0f);

    signal_ema_update(&ema, -70.0f);
    float v = signal_ema_update(&ema, -40.0f);
    REQUIRE_THAT(v, WithinAbs(-40.0f, 0.01f));
}

TEST_CASE("EMA reset restores initial state", "[signal][ema]") {
    SignalEma ema;
    signal_ema_init(&ema, 0.5f);

    signal_ema_update(&ema, -60.0f);
    signal_ema_update(&ema, -50.0f);
    signal_ema_reset(&ema);

    float v = signal_ema_update(&ema, -80.0f);
    REQUIRE_THAT(v, WithinAbs(-80.0f, 0.01f));
}

TEST_CASE("RssiAccum basic averaging", "[signal][rssi]") {
    SignalRssiAccum accum;
    signal_rssi_accum_init(&accum, 3);

    REQUIRE(signal_rssi_accum_add(&accum, -60) == 0);
    REQUIRE(signal_rssi_accum_add(&accum, -50) == 0);
    REQUIRE(signal_rssi_accum_add(&accum, -70) == 1);

    int8_t avg = signal_rssi_accum_average(&accum);
    REQUIRE(avg == -60);
}

TEST_CASE("RssiAccum reset", "[signal][rssi]") {
    SignalRssiAccum accum;
    signal_rssi_accum_init(&accum, 2);

    signal_rssi_accum_add(&accum, -40);
    signal_rssi_accum_add(&accum, -80);
    signal_rssi_accum_reset(&accum);

    REQUIRE(signal_rssi_accum_add(&accum, -55) == 0);
    REQUIRE(signal_rssi_accum_add(&accum, -65) == 1);

    int8_t avg = signal_rssi_accum_average(&accum);
    REQUIRE(avg == -60);
}

TEST_CASE("RssiAccum overflow clamped", "[signal][rssi]") {
    SignalRssiAccum accum;
    signal_rssi_accum_init(&accum, 100);

    for (int32_t i = 0; i < SIGNAL_RSSI_MAX_SAMPLES; ++i) {
        signal_rssi_accum_add(&accum, -50);
    }

    int8_t avg = signal_rssi_accum_average(&accum);
    REQUIRE(avg == -50);
}
