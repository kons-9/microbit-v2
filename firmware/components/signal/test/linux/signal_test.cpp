#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "signal_proc.h"

using Catch::Matchers::WithinAbs;

TEST_CASE("EMA basic smoothing", "[signal][ema]") {
    signal::Ema ema(0.3f);

    float v = ema.update(-60.0f);
    REQUIRE_THAT(v, WithinAbs(-60.0f, 0.01f));

    v = ema.update(-50.0f);
    // expected: 0.3*(-50) + 0.7*(-60) = -57
    REQUIRE_THAT(v, WithinAbs(-57.0f, 0.01f));
}

TEST_CASE("EMA alpha=1 means no smoothing", "[signal][ema]") {
    signal::Ema ema(1.0f);

    ema.update(-70.0f);
    float v = ema.update(-40.0f);
    REQUIRE_THAT(v, WithinAbs(-40.0f, 0.01f));
}

TEST_CASE("EMA reset restores initial state", "[signal][ema]") {
    signal::Ema ema(0.5f);

    ema.update(-60.0f);
    ema.update(-50.0f);
    ema.reset();

    float v = ema.update(-80.0f);
    REQUIRE_THAT(v, WithinAbs(-80.0f, 0.01f));
}

TEST_CASE("RssiAccum basic averaging", "[signal][rssi]") {
    signal::RssiAccum accum(3);

    REQUIRE(accum.add(-60) == false);
    REQUIRE(accum.add(-50) == false);
    REQUIRE(accum.add(-70) == true);

    int8_t avg = accum.average();
    REQUIRE(avg == -60);
}

TEST_CASE("RssiAccum reset", "[signal][rssi]") {
    signal::RssiAccum accum(2);

    accum.add(-40);
    accum.add(-80);
    accum.reset();

    REQUIRE(accum.add(-55) == false);
    REQUIRE(accum.add(-65) == true);

    int8_t avg = accum.average();
    REQUIRE(avg == -60);
}

TEST_CASE("RssiAccum overflow clamped", "[signal][rssi]") {
    signal::RssiAccum accum(100);

    for (int32_t i = 0; i < signal::RSSI_MAX_SAMPLES; ++i) {
        accum.add(-50);
    }

    int8_t avg = accum.average();
    REQUIRE(avg == -50);
}
