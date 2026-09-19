/**
 * @file uart_microbit.cpp
 * @brief micro:bit v2.2 UART (nRF52833 UARTE0) 実装
 */

#include "arch/uart_arch.h"
#include "microbit_driver_config.h"

#include "nrf.h"
#include "nrf_gpio.h"

/*
 * UARTEレジスタへの参照とEasyDMA用送信バッファは、転送処理をまたいで
 * 保持する必要がある。公開APIに含まれないドライバ内部状態としてまとめる。
 */
struct InnerState {
    struct Hardware {
        NRF_UARTE_Type *const uarte = NRF_UARTE0;
    } hardware;

    struct Dma {
        alignas(4) uint8_t tx_buffer[drivers::microbit::config::Uart::TxChunkSize] = {};
    } dma;
};

static InnerState s_state{};

static constexpr uint32_t convert_baudrate(uint32_t baud) {
    switch (baud) {
    case 9600: return UARTE_BAUDRATE_BAUDRATE_Baud9600;
    case 115200: return UARTE_BAUDRATE_BAUDRATE_Baud115200;
    default: return UARTE_BAUDRATE_BAUDRATE_Baud115200;
    }
}

int32_t uart_arch_init(uint32_t baudrate) {
    nrf_gpio_pin_set(drivers::microbit::config::Uart::TxPin);
    nrf_gpio_cfg_output(drivers::microbit::config::Uart::TxPin);
    nrf_gpio_cfg_input(drivers::microbit::config::Uart::RxPin, NRF_GPIO_PIN_NOPULL);

    s_state.hardware.uarte->PSEL.TXD = drivers::microbit::config::Uart::TxPin;
    s_state.hardware.uarte->PSEL.RXD = drivers::microbit::config::Uart::RxPin;
    s_state.hardware.uarte->PSEL.CTS = 0xFFFFFFFF;
    s_state.hardware.uarte->PSEL.RTS = 0xFFFFFFFF;

    s_state.hardware.uarte->BAUDRATE = convert_baudrate(baudrate);
    s_state.hardware.uarte->CONFIG = 0;
    s_state.hardware.uarte->ENABLE = UARTE_ENABLE_ENABLE_Enabled;

    return 0;
}

int32_t uart_arch_write(const uint8_t *data, size_t len) {
    size_t sent = 0;
    while (sent < len) {
        size_t chunk_len = len - sent;
        if (chunk_len > drivers::microbit::config::Uart::TxChunkSize) {
            chunk_len = drivers::microbit::config::Uart::TxChunkSize;
        }

        /* UARTE EasyDMA buffers must be in RAM. */
        for (size_t i = 0; i < chunk_len; ++i) {
            s_state.dma.tx_buffer[i] = data[sent + i];
        }

        s_state.hardware.uarte->TXD.PTR = reinterpret_cast<uint32_t>(s_state.dma.tx_buffer);
        s_state.hardware.uarte->TXD.MAXCNT = static_cast<uint32_t>(chunk_len);

        s_state.hardware.uarte->EVENTS_ENDTX = 0;
        s_state.hardware.uarte->TASKS_STARTTX = 1;

        while (s_state.hardware.uarte->EVENTS_ENDTX == 0) {
            /* busy wait */
        }

        sent += chunk_len;
    }

    return static_cast<int32_t>(len);
}

int32_t uart_arch_read(uint8_t *buf, size_t buf_len, uint32_t timeout_ms) {
    /*
     * Shell::poll() uses timeout_ms == 0.  UARTE completes an EasyDMA
     * receive only after MAXCNT bytes, so using the caller's 32-byte buffer
     * here would block until 32 bytes arrive.  Recovery runs from usermain()
     * directly, so collect one line a byte at a time and return it after a
     * newline (or when the buffer is full).
     */
    /* TODO: RTOS タイマによるタイムアウト実装 */
    (void)timeout_ms;

    if (timeout_ms == 0) {
        size_t received = 0;
        while (received < buf_len) {
            s_state.hardware.uarte->RXD.PTR = reinterpret_cast<uint32_t>(buf + received);
            s_state.hardware.uarte->RXD.MAXCNT = 1;

            s_state.hardware.uarte->EVENTS_ENDRX = 0;
            s_state.hardware.uarte->TASKS_STARTRX = 1;

            while (s_state.hardware.uarte->EVENTS_ENDRX == 0) {
                /* busy wait */
            }

            ++received;
            if (buf[received - 1] == '\r' || buf[received - 1] == '\n') {
                break;
            }
        }
        return static_cast<int32_t>(received);
    }

    s_state.hardware.uarte->RXD.PTR = reinterpret_cast<uint32_t>(buf);
    s_state.hardware.uarte->RXD.MAXCNT = static_cast<uint32_t>(buf_len);

    s_state.hardware.uarte->EVENTS_ENDRX = 0;
    s_state.hardware.uarte->TASKS_STARTRX = 1;

    while (s_state.hardware.uarte->EVENTS_ENDRX == 0) {
        /* busy wait */
    }

    return static_cast<int32_t>(s_state.hardware.uarte->RXD.AMOUNT);
}
