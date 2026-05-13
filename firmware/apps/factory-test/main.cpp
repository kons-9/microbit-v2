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
#include <uart.h>

#include <nrf.h>
#include <tk/tkernel.h>
#include <tk/syslib.h>

#include <cstdint>
#include <cstring>

/* ==================================================================
 * Peripheral instances
 * ================================================================== */

static drivers::Led s_led;
static drivers::Speaker s_speaker;
static drivers::Microphone s_mic;
static drivers::Accelerometer s_accel;
static drivers::Magnetometer s_mag;
static drivers::Button s_button;
static drivers::Touch s_touch;
static drivers::Temperature s_temp;
static drivers::Uart s_uart;

/* ==================================================================
 * LED scan timer (TIMER2)
 * ================================================================== */

static constexpr uint32_t LED_SCAN_INTERVAL_US = 2000;
static constexpr uint32_t LED_TIMER_IRQ_PRIORITY = 3;

static void led_timer_isr(UINT intno) {
    (void)intno;
    if (NRF_TIMER2->EVENTS_COMPARE[0] == 0) {
        return;
    }
    NRF_TIMER2->EVENTS_COMPARE[0] = 0;
    s_led.scan_tick();
}

static bool led_timer_init(void) {
    T_DINT dint;
    dint.intatr = TA_HLNG;
    dint.inthdr = reinterpret_cast<FP>(led_timer_isr);
    if (auto err = tk_def_int(TIMER2_IRQn, &dint); err < E_OK) {
        LOG_E("tk_def_int(TIMER2) failed: %ld", static_cast<int32_t>(err));
        return false;
    }

    NRF_TIMER2->TASKS_STOP = 1;
    NRF_TIMER2->TASKS_CLEAR = 1;
    NRF_TIMER2->MODE = TIMER_MODE_MODE_Timer;
    NRF_TIMER2->BITMODE = TIMER_BITMODE_BITMODE_32Bit;
    NRF_TIMER2->PRESCALER = 4; /* 16 MHz / 2^4 = 1 MHz */
    NRF_TIMER2->CC[0] = LED_SCAN_INTERVAL_US;
    NRF_TIMER2->SHORTS = TIMER_SHORTS_COMPARE0_CLEAR_Msk;
    NRF_TIMER2->INTENSET = TIMER_INTENSET_COMPARE0_Msk;

    EnableInt(TIMER2_IRQn, LED_TIMER_IRQ_PRIORITY);
    NRF_TIMER2->TASKS_START = 1;

    LOG_D("LED timer started (TIMER2, %lu us/row)", LED_SCAN_INTERVAL_US);
    return true;
}

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

static uint8_t s_frameBuf[drivers::LED_ROWS];

static void clear_frame() {
    std::memset(s_frameBuf, 0, sizeof(s_frameBuf));
    s_led.set_frame(s_frameBuf);
}

static void show_char(char c) {
    const auto *pat = led_font_get(c);
    if (pat != nullptr) {
        s_led.set_frame(pat);
    }
}

static void show_check() {
    s_led.set_frame(LED_SYM_CHECK);
}

static void show_cross() {
    s_led.set_frame(LED_SYM_CROSS);
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
    s_led.set_frame(s_frameBuf);
}

static void show_result_row(int32_t row, TestResult result) {
    s_frameBuf[row] = (result == TestResult::Pass) ? 0x1F : 0x04;
    s_led.set_frame(s_frameBuf);
}

/* ==================================================================
 * ユーザ判定 (ボタン A=Pass, B=Fail)
 * ================================================================== */

static TestResult wait_user_judgment() {
    LOG_D("waiting for user judgment: A=Pass, B=Fail");
    for (;;) {
        auto btn = s_button.wait_any(0);
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
    s_led.set_frame(LED_SYM_FULL);
    osal::task::sleep_for(500);

    s_led.clear();
    osal::task::sleep_for(500);

    for (int32_t r = 0; r < drivers::LED_ROWS; ++r) {
        clear_frame();
        s_frameBuf[r] = 0x1F;
        s_led.set_frame(s_frameBuf);
        osal::task::sleep_for(50);
    }
    for (int32_t r = 0; r < drivers::LED_ROWS; ++r) {
        clear_frame();
        s_frameBuf[drivers::LED_ROWS - 1 - r] = 0x1F;
        s_led.set_frame(s_frameBuf);
        osal::task::sleep_for(50);
    }
    for (int32_t c = 0; c < drivers::LED_COLS; ++c) {
        clear_frame();
        for (int32_t r = 0; r < drivers::LED_ROWS; ++r) {
            s_frameBuf[r] = static_cast<uint8_t>(1U << c);
        }
        s_led.set_frame(s_frameBuf);
        osal::task::sleep_for(50);
    }
    for (int32_t c = 0; c < drivers::LED_COLS; ++c) {
        clear_frame();
        for (int32_t r = 0; r < drivers::LED_ROWS; ++r) {
            s_frameBuf[r] |= static_cast<uint8_t>(1U << (drivers::LED_COLS - 1 - c));
        }
        s_led.set_frame(s_frameBuf);
        osal::task::sleep_for(50);
    }
    clear_frame();
    for (int32_t r = 0; r < drivers::LED_ROWS; ++r) {
        for (int32_t c = 0; c < drivers::LED_COLS; ++c) {
            s_led.set(r, c, true);
            osal::task::sleep_for(10);
        }
    }
    for (int32_t r = 0; r < drivers::LED_ROWS; ++r) {
        for (int32_t c = 0; c < drivers::LED_COLS; ++c) {
            s_led.set(r, c, false);
            osal::task::sleep_for(10);
        }
    }
    for (int32_t r = 0; r < 10; ++r) {
        {
            const uint8_t checker[5] = {0x15, 0x0A, 0x15, 0x0A, 0x15};
            s_led.set_frame(checker);
        }
        osal::task::sleep_for(100);
        {
            const uint8_t checker[5] = {0x0A, 0x15, 0x0A, 0x15, 0x0A};
            s_led.set_frame(checker);
        }
        osal::task::sleep_for(100);
    }
    clear_frame();

    return wait_user_judgment();
}

static TestResult test_speaker() {
    s_speaker.tone(1000);
    osal::task::sleep_for(500);

    s_speaker.tone(2700);
    osal::task::sleep_for(500);

    s_speaker.stop();

    return wait_user_judgment();
}

static TestResult test_microphone() {
    s_mic.enable();
    osal::task::sleep_for(50);

    LOG_I("mic: sampling baseline...");
    uint32_t baseline = 0;
    for (int32_t i = 0; i < 16; ++i) {
        uint16_t sample = s_mic.read();
        LOG_D("mic: sample[%ld]=%u", i, sample);
        baseline += sample;
        osal::task::sleep_for(6);
    }
    baseline /= 16;
    LOG_I("mic: baseline=%lu", baseline);

    LOG_I("mic: playing 2700Hz tone, sampling...");
    s_speaker.tone(2700);
    osal::task::sleep_for(100);

    uint32_t activeLevel = 0;
    for (int32_t i = 0; i < 16; ++i) {
        uint16_t sample = s_mic.read();
        LOG_D("mic: active_sample[%ld]=%u", i, sample);
        activeLevel += sample;
        osal::task::sleep_for(6);
    }
    activeLevel /= 16;

    s_speaker.stop();
    s_mic.disable();

    int32_t diff = static_cast<int32_t>(activeLevel) - static_cast<int32_t>(baseline);
    LOG_I("mic: active=%lu, diff=%ld", activeLevel, diff);
    LOG_I("mic: A=Pass, B=Fail");

    return wait_user_judgment();
}

static TestResult test_accelerometer() {
    uint8_t id = s_accel.who_am_i();
    LOG_I("accel: WHO_AM_I=0x%02x (expect 0x33)", id);
    if (id != 0x33) {
        LOG_E("accel: WHO_AM_I failed");
        return TestResult::Fail;
    }

    for (int32_t t = 0; t < 20; ++t) {
        auto d = s_accel.read();
        LOG_I("accel: x=%d y=%d z=%d", d.m_x, d.m_y, d.m_z);
        show_tilt(d.m_x, d.m_y);
        osal::task::sleep_for(100);
    }

    LOG_I("accel: A=Pass, B=Fail");
    return wait_user_judgment();
}

static TestResult test_magnetometer() {
    uint8_t id = s_mag.who_am_i();
    LOG_I("mag: WHO_AM_I=0x%02x (expect 0x40)", id);
    if (id != 0x40) {
        LOG_E("mag: WHO_AM_I failed");
        return TestResult::Fail;
    }

    for (int32_t t = 0; t < 10; ++t) {
        auto m = s_mag.read();
        LOG_I("mag: x=%d y=%d z=%d", m.m_x, m.m_y, m.m_z);
        osal::task::sleep_for(200);
    }

    LOG_I("mag: A=Pass, B=Fail");
    return wait_user_judgment();
}

static TestResult test_buttons() {
    show_char('A');
    LOG_I("btn: press A (15s timeout)");
    if (!s_button.wait_press(0, 15000)) {
        return TestResult::Fail;
    }

    show_char('B');
    LOG_I("btn: press B (15s timeout)");
    if (!s_button.wait_press(1, 15000)) {
        return TestResult::Fail;
    }

    return TestResult::Pass;
}

static TestResult test_touch() {
    show_char('T');
    LOG_I("touch: touch the logo (15s timeout)");
    if (s_touch.wait(15000)) {
        LOG_I("touch: detected");
        return TestResult::Pass;
    }
    LOG_I("touch: timeout, A=Pass, B=Fail");
    return wait_user_judgment();
}

static TestResult test_temperature() {
    auto temp = s_temp.read();
    LOG_I("temp: %ld deg C", static_cast<int32_t>(temp));
    LOG_I("temp: A=Pass, B=Fail");
    return wait_user_judgment();
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
    s_uart.init();
    LogInit(LOG_LEVEL_DEBUG, s_uart);
    LOG_I("factory-test start");

    s_led.init();
    led_timer_init();
    s_speaker.init();
    s_mic.init();
    s_accel.init();
    s_mag.init();
    s_button.init();
    s_touch.init();
    s_temp.init();

    LOG_D("all peripherals initialized");

    TestResult results[NUM_TESTS];
    bool allPass = true;

    for (int32_t i = 0; i < NUM_TESTS; ++i) {
        LOG_D("test[%ld] %s begin", i, s_tests[i].m_name);
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
