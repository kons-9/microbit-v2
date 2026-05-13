/**
 * @file touch_microbit.cpp
 * @brief micro:bit v2.2 タッチロゴ GPIO ポーリング実装
 */

#include "touch.h"

#define LOG_TAG "TOUCH"
#include "log.h"

#include "nrf_gpio.h"

#include <tk/tkernel.h>

static constexpr uint32_t FACE_TOUCH_PIN = NRF_GPIO_PIN_MAP(1, 4);
static constexpr uint32_t DEBOUNCE_MS = 50;
static constexpr uint32_t POLL_INTERVAL_MS = 10;

namespace drivers {

void Touch::init() {
    nrf_gpio_cfg_input(FACE_TOUCH_PIN, NRF_GPIO_PIN_NOPULL);
    LOG_D("init: LOGO=P1.04");
}

bool Touch::is_touched() {
    return (nrf_gpio_pin_read(FACE_TOUCH_PIN) == 0);
}

bool Touch::wait(uint32_t timeout_ms) {
    uint32_t elapsed = 0;

    while (timeout_ms == 0 || elapsed < timeout_ms) {
        if (is_touched()) {
            if (auto er = tk_dly_tsk(DEBOUNCE_MS); er < E_OK) {
                LOG_E("tk_dly_tsk failed: %ld", static_cast<int32_t>(er));
                return false;
            }
            if (is_touched()) {
                return true;
            }
        }
        if (auto er = tk_dly_tsk(POLL_INTERVAL_MS); er < E_OK) {
            LOG_E("tk_dly_tsk failed: %ld", static_cast<int32_t>(er));
            return false;
        }
        elapsed += POLL_INTERVAL_MS;
    }
    return false;
}

}  // namespace drivers
