#include "touch.h"
#include "nrf_gpio.h"
#include <tk/tkernel.h>

// micro:bit v2.2 touch logo pin
#define FACE_TOUCH_PIN NRF_GPIO_PIN_MAP(1, 4)

void touch_init(void) {
    // 入力設定 (外部10Mohmプルアップあり、内部プルアップ不要)
    nrf_gpio_cfg_input(FACE_TOUCH_PIN, NRF_GPIO_PIN_NOPULL);
}

bool touch_is_touched(void) {
    // active low: タッチ時にLOW
    return (nrf_gpio_pin_read(FACE_TOUCH_PIN) == 0);
}

bool touch_wait(uint32_t timeout_ms) {
    uint32_t elapsed = 0;
    const uint32_t poll_interval = 10;

    while (timeout_ms == 0 || elapsed < timeout_ms) {
        if (touch_is_touched()) {
            // デバウンス
            tk_dly_tsk(50);
            if (touch_is_touched()) {
                return true;
            }
        }
        tk_dly_tsk(poll_interval);
        elapsed += poll_interval;
    }
    return false;
}
