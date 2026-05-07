#include "temperature.h"
#include "nrf.h"

void temperature_init(void) {
    // TEMP peripheralは特別な初期化不要
    // 測定はオンデマンドで実行
}

int32_t temperature_read_raw(void) {
    // 測定開始
    NRF_TEMP->TASKS_START = 1;

    // 測定完了待ち
    while (NRF_TEMP->EVENTS_DATARDY == 0) {
        // busy wait (典型的に36μs)
    }
    NRF_TEMP->EVENTS_DATARDY = 0;

    // 結果取得 (0.25°C単位)
    int32_t raw = NRF_TEMP->TEMP;

    // 測定停止
    NRF_TEMP->TASKS_STOP = 1;

    return raw;
}

int8_t temperature_read(void) {
    return (int8_t)(temperature_read_raw() / 4);
}
