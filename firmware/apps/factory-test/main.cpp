/**
 * @file main.cpp
 * @brief ファクトリーテスト アプリケーション
 *
 * 各ハードウェアコンポーネントを順次テストし、
 * LED で結果を表示する。
 */

#define LOG_TAG "TEST"
#include "log.h"

#include <osal/task>
#include <led.h>
#include <led_font.h>
#include <speaker.h>
#include <mic.h>
#include <accelerometer.h>
#include <magnetometer.h>
#include <button.h>
#include <touch.h>
#include <temperature.h>

#include <cstdint>
#include <cstring>

/* ==================================================================
 * テスト結果
 * ================================================================== */

enum class TestResult : uint8_t {
    Pass,
    Fail,
    Skip,
};

/* ==================================================================
 * LED 表示ヘルパー
 * ================================================================== */

static uint8_t s_frameBuf[LED_ROWS];

static void clear_frame() {
    std::memset(s_frameBuf, 0, sizeof(s_frameBuf));
    led_set_frame(s_frameBuf);
}

static void fill_frame() {
    led_set_frame(LED_SYM_FULL);
}

static void show_char(char c) {
    const auto *pat = led_font_get(c);
    if (pat != nullptr) {
        led_set_frame(pat);
    }
}

static void show_check() {
    led_set_frame(LED_SYM_CHECK);
}

static void show_cross() {
    led_set_frame(LED_SYM_CROSS);
}

static void show_test_number(int32_t n) {
    if (n >= 0 && n <= 9) {
        show_char(static_cast<char>('0' + n));
    } else {
        clear_frame();
        s_frameBuf[0] = static_cast<uint8_t>(n & 0x1F);
        led_set_frame(s_frameBuf);
    }
}

static void show_bar(int32_t level) {
    clear_frame();
    for (int32_t r = 4; r >= 0 && r > (4 - level); --r) {
        s_frameBuf[r] = 0x1F;
    }
    led_set_frame(s_frameBuf);
}

static void show_tilt(int16_t x_mg, int16_t y_mg) {
    clear_frame();
    int32_t dx = x_mg / 400;
    int32_t dy = y_mg / 400;
    if (dx < -2) {
        dx = -2;
    }
    if (dx > 2) {
        dx = 2;
    }
    if (dy < -2) {
        dy = -2;
    }
    if (dy > 2) {
        dy = 2;
    }
    int32_t col = 2 + dx;
    int32_t row = 2 + dy;
    s_frameBuf[row] = static_cast<uint8_t>(1 << col);
    led_set_frame(s_frameBuf);
}

static void show_result_row(int32_t row, TestResult result) {
    s_frameBuf[row] = (result == TestResult::Pass) ? 0x1F : 0x04;
    led_set_frame(s_frameBuf);
}

/* ==================================================================
 * ユーザ判定 (ボタン A=Pass, B=Fail)
 * ================================================================== */

static TestResult wait_user_judgment() {
    for (;;) {
        auto btn = button_wait_any(0);
        if (btn == 0) {
            return TestResult::Pass;
        }
        if (btn == 1) {
            return TestResult::Fail;
        }
    }
}

/* ==================================================================
 * テスト関数群
 * ================================================================== */

static TestResult test_led() {
    fill_frame();
    osal::task::sleep_for(1000);

    clear_frame();
    osal::task::sleep_for(500);

    for (int32_t r = 0; r < LED_ROWS; ++r) {
        clear_frame();
        s_frameBuf[r] = 0x1F;
        led_set_frame(s_frameBuf);
        osal::task::sleep_for(300);
    }

    const uint8_t checker[5] = {0x15, 0x0A, 0x15, 0x0A, 0x15};
    led_set_frame(checker);
    osal::task::sleep_for(1000);

    return wait_user_judgment();
}

static TestResult test_speaker() {
    speaker_tone(1000);
    osal::task::sleep_for(500);

    speaker_tone(2700);
    osal::task::sleep_for(500);

    speaker_stop();

    return wait_user_judgment();
}

static TestResult test_microphone() {
    microphone_enable();
    osal::task::sleep_for(50);

    uint32_t baseline = 0;
    for (int32_t i = 0; i < 16; ++i) {
        baseline += microphone_read();
        osal::task::sleep_for(6);
    }
    baseline /= 16;

    for (int32_t t = 0; t < 40; ++t) {
        uint16_t val = microphone_read();
        int32_t level = (val > baseline) ? static_cast<int32_t>(val - baseline) / 50 : 0;
        if (level > 5) {
            level = 5;
        }
        show_bar(level);
        osal::task::sleep_for(50);
    }

    speaker_tone(2700);
    osal::task::sleep_for(50);

    uint32_t activeLevel = 0;
    for (int32_t i = 0; i < 16; ++i) {
        activeLevel += microphone_read();
        osal::task::sleep_for(6);
    }
    activeLevel /= 16;

    speaker_stop();
    microphone_disable();

    int32_t diff = static_cast<int32_t>(activeLevel) - static_cast<int32_t>(baseline);
    return (diff > 30) ? TestResult::Pass : TestResult::Fail;
}

static TestResult test_accelerometer() {
    if (accelerometer_who_am_i() != 0x33) {
        return TestResult::Fail;
    }

    int32_t xSum = 0, ySum = 0, zSum = 0;
    for (int32_t i = 0; i < 10; ++i) {
        auto d = accelerometer_read();
        xSum += d.m_x;
        ySum += d.m_y;
        zSum += d.m_z;
        osal::task::sleep_for(20);
    }
    auto xAvg = static_cast<int16_t>(xSum / 10);
    auto yAvg = static_cast<int16_t>(ySum / 10);
    auto zAvg = static_cast<int16_t>(zSum / 10);

    for (int32_t t = 0; t < 40; ++t) {
        auto d = accelerometer_read();
        show_tilt(d.m_x, d.m_y);
        osal::task::sleep_for(50);
    }

    bool pass = (zAvg > 800 && zAvg < 1200) && (xAvg > -300 && xAvg < 300) && (yAvg > -300 && yAvg < 300);
    return pass ? TestResult::Pass : TestResult::Fail;
}

static TestResult test_magnetometer() {
    if (magnetometer_who_am_i() != 0x40) {
        return TestResult::Fail;
    }

    auto m = magnetometer_read();
    int32_t magSq = static_cast<int32_t>(m.m_x) * m.m_x + static_cast<int32_t>(m.m_y) * m.m_y
                    + static_cast<int32_t>(m.m_z) * m.m_z;

    bool pass = (magSq > 4000000) && (magSq < 64000000);
    return pass ? TestResult::Pass : TestResult::Fail;
}

static TestResult test_buttons() {
    show_char('A');
    if (!button_wait_press(0, 5000)) {
        return TestResult::Fail;
    }

    show_char('B');
    if (!button_wait_press(1, 5000)) {
        return TestResult::Fail;
    }

    return TestResult::Pass;
}

static TestResult test_touch() {
    show_char('T');
    return touch_wait(5000) ? TestResult::Pass : TestResult::Fail;
}

static TestResult test_temperature() {
    auto temp = temperature_read();
    return (temp > 10 && temp < 50) ? TestResult::Pass : TestResult::Fail;
}

/* ==================================================================
 * テストランナー
 * ================================================================== */

using TestFunc = TestResult (*)();

struct TestEntry {
    const char *m_name;
    TestFunc m_func;
};

static const TestEntry s_tests[] = {
    {"LED", test_led},
    {"Speaker", test_speaker},
    {"Mic", test_microphone},
    {"Accel", test_accelerometer},
    {"Mag", test_magnetometer},
    {"Button", test_buttons},
    {"Touch", test_touch},
    {"Temperature", test_temperature},
};

static constexpr int32_t NUM_TESTS = sizeof(s_tests) / sizeof(s_tests[0]);

/* ==================================================================
 * Entry Point
 * ================================================================== */

static int app_main();

extern "C" int usermain(void) {
    return app_main();
}

static int app_main() {
    LogInit(LOG_LEVEL_DEBUG);
    LOG_I("factory-test start");

    led_init();
    speaker_init();
    microphone_init();
    accelerometer_init();
    magnetometer_init();
    button_init();
    touch_init();
    temperature_init();

    LOG_D("all peripherals initialized");

    TestResult results[NUM_TESTS];
    bool allPass = true;

    for (int32_t i = 0; i < NUM_TESTS; ++i) {
        LOG_D("test[%ld] %s begin", i, s_tests[i].m_name);
        show_test_number(i + 1);
        osal::task::sleep_for(500);

        results[i] = s_tests[i].m_func();

        if (results[i] != TestResult::Pass) {
            allPass = false;
            LOG_W("test[%ld] %s FAIL", i, s_tests[i].m_name);
        } else {
            LOG_D("test[%ld] %s PASS", i, s_tests[i].m_name);
        }

        if (i < 4) {
            show_result_row(i + 1, results[i]);
        }
        osal::task::sleep_for(500);
        osal::task::sleep_for(300);
    }

    clear_frame();
    osal::task::sleep_for(200);

    if (allPass) {
        show_check();
        LOG_I("all tests PASSED");
    } else {
        show_cross();
        LOG_W("some tests FAILED");
    }

    for (;;) {
        osal::task::sleep_for(1000);
    }

    return 0;
}
