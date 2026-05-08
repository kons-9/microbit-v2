#include <osal/task>
#include <osal/timer>
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

// ----------------------------------------------------------
// テスト結果定義
// ----------------------------------------------------------
enum TestResult {
    TEST_PASS,
    TEST_FAIL,
    TEST_SKIP
};

// ----------------------------------------------------------
// LED表示ヘルパー
// ----------------------------------------------------------
static uint8_t frame_buf[LED_ROWS];

static void frame_clear() {
    memset(frame_buf, 0, sizeof(frame_buf));
    led_set_frame(frame_buf);
}

static void frame_fill() {
    led_set_frame(LED_SYM_FULL);
}

static void show_char(char c) {
    const uint8_t *pat = led_font_get(c);
    if (pat)
        led_set_frame(pat);
}

static void show_check() {
    led_set_frame(LED_SYM_CHECK);
}

static void show_cross() {
    led_set_frame(LED_SYM_CROSS);
}

static void show_test_number(int n) {
    if (n >= 0 && n <= 9) {
        show_char('0' + n);
    } else {
        frame_clear();
        frame_buf[0] = static_cast<uint8_t>(n & 0x1F);
        led_set_frame(frame_buf);
    }
}

// LEDバーグラフ表示 (0-4レベル、下から上に点灯)
static void show_bar(int level) {
    frame_clear();
    for (int r = 4; r >= 0 && r > (4 - level); --r) {
        frame_buf[r] = 0x1F;
    }
    led_set_frame(frame_buf);
}

// 加速度の傾きを2Dドットで表示
static void show_tilt(int16_t x_mg, int16_t y_mg) {
    frame_clear();
    // 中央(2,2)を基準に、±1000mgを±2ドットに変換
    int dx = x_mg / 400;
    int dy = y_mg / 400;
    if (dx < -2)
        dx = -2;
    if (dx > 2)
        dx = 2;
    if (dy < -2)
        dy = -2;
    if (dy > 2)
        dy = 2;
    int col = 2 + dx;
    int row = 2 + dy;
    frame_buf[row] = static_cast<uint8_t>(1 << col);
    led_set_frame(frame_buf);
}

// 結果をLED行で表示
static void show_result_row(int row, TestResult result) {
    if (result == TEST_PASS) {
        frame_buf[row] = 0x1F;  // 全点灯
    } else {
        // FAIL: 中央のみ
        frame_buf[row] = 0x04;
    }
    led_set_frame(frame_buf);
}

// ----------------------------------------------------------
// ボタンによるPASS/FAIL判定 (目視テスト用)
// ボタンA = PASS, ボタンB = FAIL (再実行なし、シンプル版)
// ----------------------------------------------------------
static TestResult wait_user_judgment() {
    for (;;) {
        button_id_t btn = button_wait_any(0);
        if (btn == BUTTON_A)
            return TEST_PASS;
        if (btn == BUTTON_B)
            return TEST_FAIL;
    }
}

// ----------------------------------------------------------
// [1] LED テスト
// ----------------------------------------------------------
static TestResult test_led() {
    // 全LED点灯
    frame_fill();
    osal::task::sleep_for(1000);

    // 全LED消灯
    frame_clear();
    osal::task::sleep_for(500);

    // 1行ずつスキャン
    for (int r = 0; r < LED_ROWS; ++r) {
        frame_clear();
        frame_buf[r] = 0x1F;
        led_set_frame(frame_buf);
        osal::task::sleep_for(300);
    }

    // チェッカーパターン
    const uint8_t checker[5] = {0x15, 0x0A, 0x15, 0x0A, 0x15};
    led_set_frame(checker);
    osal::task::sleep_for(1000);

    // ユーザ判定を待つ
    return wait_user_judgment();
}

// ----------------------------------------------------------
// [2] Speaker テスト
// ----------------------------------------------------------
static TestResult test_speaker() {
    // 1000Hz トーン再生
    speaker_tone(1000);
    osal::task::sleep_for(500);

    // 2700Hz (共振周波数) 再生
    speaker_tone(2700);
    osal::task::sleep_for(500);

    // 停止
    speaker_stop();

    // ユーザ判定を待つ
    return wait_user_judgment();
}

// ----------------------------------------------------------
// [3] Mic テスト
// ----------------------------------------------------------
static TestResult test_mic() {
    mic_enable();
    osal::task::sleep_for(50);  // 安定待ち

    // 無音時のベースライン取得 (16サンプル平均)
    uint32_t baseline = 0;
    for (int i = 0; i < 16; ++i) {
        baseline += mic_read();
        osal::task::sleep_for(6);  // ~100ms / 16
    }
    baseline /= 16;

    // リアルタイムLED表示期間 (ユーザがマイクの動作を確認できるよう2秒間)
    for (int t = 0; t < 40; ++t) {
        uint16_t val = mic_read();
        int level = (val > baseline) ? (val - baseline) / 50 : 0;
        if (level > 5)
            level = 5;
        show_bar(level);
        osal::task::sleep_for(50);
    }

    // Speaker 2700Hz再生してマイク検証
    speaker_tone(2700);
    osal::task::sleep_for(50);  // 安定待ち

    uint32_t active_level = 0;
    for (int i = 0; i < 16; ++i) {
        active_level += mic_read();
        osal::task::sleep_for(6);
    }
    active_level /= 16;

    speaker_stop();
    mic_disable();

    // 判定
    int32_t diff = static_cast<int32_t>(active_level) - static_cast<int32_t>(baseline);
    return (diff > 30) ? TEST_PASS : TEST_FAIL;
}

// ----------------------------------------------------------
// [4] 加速度センサ テスト
// ----------------------------------------------------------
static TestResult test_accelerometer() {
    // WHO_AM_I確認
    uint8_t who = accel_who_am_i();
    if (who != 0x33) {
        return TEST_FAIL;
    }

    // 加速度読み取り (10回平均)
    int32_t x_sum = 0, y_sum = 0, z_sum = 0;
    for (int i = 0; i < 10; ++i) {
        accel_data_t d = accel_read();
        x_sum += d.x;
        y_sum += d.y;
        z_sum += d.z;
        osal::task::sleep_for(20);
    }
    int16_t x_avg = static_cast<int16_t>(x_sum / 10);
    int16_t y_avg = static_cast<int16_t>(y_sum / 10);
    int16_t z_avg = static_cast<int16_t>(z_sum / 10);

    // リアルタイム傾き表示 (2秒間)
    for (int t = 0; t < 40; ++t) {
        accel_data_t d = accel_read();
        show_tilt(d.x, d.y);
        osal::task::sleep_for(50);
    }

    // 静止状態判定: Z ≈ 1000mg, X/Y ≈ 0
    bool pass = (z_avg > 800 && z_avg < 1200) && (x_avg > -300 && x_avg < 300) && (y_avg > -300 && y_avg < 300);
    return pass ? TEST_PASS : TEST_FAIL;
}

// ----------------------------------------------------------
// [5] 地磁気センサ テスト
// ----------------------------------------------------------
static TestResult test_magnetometer() {
    // WHO_AM_I確認
    uint8_t who = mag_who_am_i();
    if (who != 0x40) {
        return TEST_FAIL;
    }

    // 磁場読み取り
    mag_data_t m = mag_read();

    // ベクトル長計算 (mGauss → μT: 1 mGauss = 0.1 μT)
    // magnitude in mGauss, plan says 200-800 μT → 2000-8000 mGauss
    int32_t mag_sq = (int32_t)m.x * m.x + (int32_t)m.y * m.y + (int32_t)m.z * m.z;
    // sqrt近似: 2000^2 = 4000000, 8000^2 = 64000000
    bool pass = (mag_sq > 4000000) && (mag_sq < 64000000);
    return pass ? TEST_PASS : TEST_FAIL;
}

// ----------------------------------------------------------
// [6] ボタン テスト
// ----------------------------------------------------------
static TestResult test_buttons() {
    // ボタンA待ち
    show_char('A');
    bool a_ok = button_wait_press(BUTTON_A, 5000);
    if (!a_ok)
        return TEST_FAIL;

    // ボタンB待ち
    show_char('B');
    bool b_ok = button_wait_press(BUTTON_B, 5000);
    if (!b_ok)
        return TEST_FAIL;

    return TEST_PASS;
}

// ----------------------------------------------------------
// [7] タッチロゴ テスト
// ----------------------------------------------------------
static TestResult test_touch() {
    show_char('T');
    bool touched = touch_wait(5000);
    return touched ? TEST_PASS : TEST_FAIL;
}

// ----------------------------------------------------------
// [8] 温度センサ テスト
// ----------------------------------------------------------
static TestResult test_temperature() {
    int8_t temp = temperature_read();
    return (temp > 10 && temp < 50) ? TEST_PASS : TEST_FAIL;
}

// ----------------------------------------------------------
// テストランナー
// ----------------------------------------------------------
typedef TestResult (*test_func_t)();

struct TestEntry {
    const char *name;
    test_func_t func;
};

static const TestEntry tests[] = {
    {"LED", test_led},
    {"Speaker", test_speaker},
    {"Mic", test_mic},
    {"Accel", test_accelerometer},
    {"Mag", test_magnetometer},
    {"Button", test_buttons},
    {"Touch", test_touch},
    {"Temperature", test_temperature},
};

static constexpr int NUM_TESTS = sizeof(tests) / sizeof(tests[0]);

// ----------------------------------------------------------
// LED scan タイマコールバック
// ----------------------------------------------------------
static void led_scan_callback(void *) {
    led_scan_tick();
}

// ----------------------------------------------------------
// メインエントリ
// ----------------------------------------------------------
static int _main();

extern "C" int usermain(void) {
    return _main();
}

static int _main() {
    // ハードウェア初期化
    led_init();
    speaker_init();
    mic_init();
    accel_init();
    mag_init();
    button_init();
    touch_init();
    temperature_init();

    // LED走査タイマ (3ms周期でマトリクス走査)
    osal::cyclic_timer scan_timer(led_scan_callback, 3);
    scan_timer.start();

    // テスト実行
    TestResult results[NUM_TESTS];
    bool all_pass = true;

    for (int i = 0; i < NUM_TESTS; ++i) {
        // テスト番号表示
        show_test_number(i + 1);
        osal::task::sleep_for(500);

        // テスト実行
        results[i] = tests[i].func();

        if (results[i] != TEST_PASS) {
            all_pass = false;
        }

        // 結果をLED行で表示 (row 1-4 はテスト1-4に対応)
        if (i < 4) {
            show_result_row(i + 1, results[i]);
        }
        osal::task::sleep_for(500);

        // ボタンAで次のテストへ (自動判定テストはそのまま進行)
        if (results[i] == TEST_PASS || results[i] == TEST_FAIL) {
            // 短い待機後に自動遷移
            osal::task::sleep_for(300);
        }
    }

    // 最終結果表示
    frame_clear();
    osal::task::sleep_for(200);

    if (all_pass) {
        show_check();
    } else {
        show_cross();
    }

    // 無限待機 (リセットまで結果を表示し続ける)
    for (;;) {
        osal::task::sleep_for(1000);
    }

    return 0;
}
