/**
 * @file uart_microbit.cpp
 * @brief micro:bit v2.2 UART (nRF52833 UARTE0) 実装
 */

#include "arch/uart_arch.h"

#include "nrf.h"
#include "nrf_gpio.h"

static constexpr uint32_t UART_TX_PIN = NRF_GPIO_PIN_MAP(0, 6);
static constexpr uint32_t UART_RX_PIN = NRF_GPIO_PIN_MAP(1, 8);

static NRF_UARTE_Type *const s_uarte = NRF_UARTE0;

static uint32_t convert_baudrate(uint32_t baud) {
    switch (baud) {
    case 9600: return UARTE_BAUDRATE_BAUDRATE_Baud9600;
    case 115200: return UARTE_BAUDRATE_BAUDRATE_Baud115200;
    default: return UARTE_BAUDRATE_BAUDRATE_Baud115200;
    }
}

int32_t uart_arch_init(uint32_t baudrate) {
    nrf_gpio_pin_set(UART_TX_PIN);
    nrf_gpio_cfg_output(UART_TX_PIN);
    nrf_gpio_cfg_input(UART_RX_PIN, NRF_GPIO_PIN_NOPULL);

    s_uarte->PSEL.TXD = UART_TX_PIN;
    s_uarte->PSEL.RXD = UART_RX_PIN;
    s_uarte->PSEL.CTS = 0xFFFFFFFF;
    s_uarte->PSEL.RTS = 0xFFFFFFFF;

    s_uarte->BAUDRATE = convert_baudrate(baudrate);
    s_uarte->CONFIG = 0;
    s_uarte->ENABLE = UARTE_ENABLE_ENABLE_Enabled;

    return 0;
}

int32_t uart_arch_write(const uint8_t *data, size_t len) {
    s_uarte->TXD.PTR = reinterpret_cast<uint32_t>(data);
    s_uarte->TXD.MAXCNT = static_cast<uint32_t>(len);

    s_uarte->EVENTS_ENDTX = 0;
    s_uarte->TASKS_STARTTX = 1;

    while (s_uarte->EVENTS_ENDTX == 0) {
        /* busy wait */
    }

    return static_cast<int32_t>(len);
}

int32_t uart_arch_read(uint8_t *buf, size_t buf_len, uint32_t timeout_ms) {
    s_uarte->RXD.PTR = reinterpret_cast<uint32_t>(buf);
    s_uarte->RXD.MAXCNT = static_cast<uint32_t>(buf_len);

    s_uarte->EVENTS_ENDRX = 0;
    s_uarte->TASKS_STARTRX = 1;

    /* TODO: RTOS タイマによるタイムアウト実装 */
    (void)timeout_ms;

    while (s_uarte->EVENTS_ENDRX == 0) {
        /* busy wait */
    }

    return static_cast<int32_t>(s_uarte->RXD.AMOUNT);
}
