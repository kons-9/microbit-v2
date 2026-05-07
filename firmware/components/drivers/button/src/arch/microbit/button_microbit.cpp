#include "button.h"
#include "nrf_gpio.h"
#include <tk/tkernel.h>

// micro:bit v2.2 button pins (active low with external pull-up)
#define BUTTON_A_PIN NRF_GPIO_PIN_MAP(0, 14)
#define BUTTON_B_PIN NRF_GPIO_PIN_MAP(0, 23)

static const uint32_t button_pins[] = {BUTTON_A_PIN, BUTTON_B_PIN};

void button_init(void) {
    nrf_gpio_cfg_input(BUTTON_A_PIN, NRF_GPIO_PIN_NOPULL);  // external pull-up
    nrf_gpio_cfg_input(BUTTON_B_PIN, NRF_GPIO_PIN_NOPULL);
}

bool button_is_pressed(button_id_t id) {
    if (id > BUTTON_B)
        return false;
    // active low: pressed = pin LOW
    return (nrf_gpio_pin_read(button_pins[id]) == 0);
}

bool button_wait_press(button_id_t id, uint32_t timeout_ms) {
    uint32_t elapsed = 0;
    const uint32_t poll_interval = 10;  // 10ms polling

    while (timeout_ms == 0 || elapsed < timeout_ms) {
        if (button_is_pressed(id)) {
            // デバウンス: 50ms後に再確認
            tk_dly_tsk(50);
            if (button_is_pressed(id)) {
                return true;
            }
        }
        tk_dly_tsk(poll_interval);
        elapsed += poll_interval;
    }
    return false;
}

button_id_t button_wait_any(uint32_t timeout_ms) {
    uint32_t elapsed = 0;
    const uint32_t poll_interval = 10;

    while (timeout_ms == 0 || elapsed < timeout_ms) {
        if (button_is_pressed(BUTTON_A)) {
            tk_dly_tsk(50);
            if (button_is_pressed(BUTTON_A))
                return BUTTON_A;
        }
        if (button_is_pressed(BUTTON_B)) {
            tk_dly_tsk(50);
            if (button_is_pressed(BUTTON_B))
                return BUTTON_B;
        }
        tk_dly_tsk(poll_interval);
        elapsed += poll_interval;
    }
    return BUTTON_A;  // timeout default
}
