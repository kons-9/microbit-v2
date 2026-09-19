/**
 * @file touch_microbit.cpp
 * @brief micro:bit v2.2 タッチロゴ GPIO ポーリング実装
 */

#include "touch.h"
#include "microbit_driver_config.h"

#define LOG_TAG "TOUCH"
#include "log.h"

#include "nrf_gpio.h"

#include <utkernel/task>

namespace drivers {

void Touch::init() {
    nrf_gpio_cfg_input(microbit::config::Touch::FacePin, NRF_GPIO_PIN_NOPULL);
    LOG_D("init: LOGO=P1.04");
}

bool Touch::is_touched() {
    return (nrf_gpio_pin_read(microbit::config::Touch::FacePin) == 0);
}

bool Touch::wait(uint32_t timeout_ms) {
    uint32_t elapsed = 0;

    while (timeout_ms == 0 || elapsed < timeout_ms) {
        if (is_touched()) {
            utkernel::task::sleep_for(microbit::config::Touch::DebounceMs);
            if (is_touched()) {
                return true;
            }
        }
        utkernel::task::sleep_for(microbit::config::Touch::PollIntervalMs);
        elapsed += microbit::config::Touch::PollIntervalMs;
    }
    return false;
}

}  // namespace drivers
