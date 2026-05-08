/**
 * @file ble_microbit.c
 * @brief BLE Observer/Broadcaster — nRF52833 RADIO 直叩き実装
 *
 * BLE の Advertising パケットの受信 (Observer) と送信 (Broadcaster) を
 * nRF52833 の RADIO ペリフェラルを直接操作して実現する。
 *
 * NOTE: このファイルは低レベルレジスタ操作と割り込みハンドラを含むため C で記述する。
 *
 * === スキャン (Receiver) ===
 * - RADIO を BLE_1MBIT モードで設定し、ADV チャネル (37/38/39) を巡回受信
 * - パケット受信完了 (END イベント) で割り込みハンドラが発火
 * - CRC 検証後、PDU をパースしてコールバックで上位層に通知
 *
 * === アドバタイズ (Transmitter) ===
 * - TIMER1 で Advertising Interval を制御
 * - 各 interval で ch37→38→39 に順次 ADV パケットを同期送信
 *
 * 参考資料:
 * - nRF52833 Product Specification §6.20 RADIO
 *   https://docs.nordicsemi.com/bundle/ps_nrf52833/page/radio.html
 * - Bluetooth Core Spec v5.x Vol.6 Part B §2.3 (Advertising PDU)
 */

#include "../ble_arch.h"
#include "nrf.h"

#include <string.h>

/* ==================================================================
 * 定数
 * ================================================================== */

/**
 * BLE Advertising チャネルの周波数テーブル
 *
 * nRF52 の FREQUENCY レジスタは「2400MHz + value」の形式。
 *   ch37 = 2402 MHz → FREQUENCY = 2
 *   ch38 = 2426 MHz → FREQUENCY = 26
 *   ch39 = 2480 MHz → FREQUENCY = 80
 */
static const uint8_t ADV_CHANNEL_FREQUENCY[3] = {2, 26, 80};

/**
 * Data Whitening の初期値 (チャネル番号)
 *
 * nRF52 では DATAWHITEIV レジスタに bit[6]=1 | channel_number を設定する。
 */
static const uint8_t ADV_CHANNEL_NUMBER[3] = {37, 38, 39};

/**
 * BLE Advertising の Access Address (固定値)
 *
 * nRF52 では BASE0 + PREFIX0 に分割して設定する:
 *   BASE0   = access_addr << 8 の下位32bit
 *   PREFIX0 = access_addr の最下位バイト
 */
#define BLE_ADV_ACCESS_ADDRESS 0x8E89BED6U

/** 受信バッファサイズ (PDU最大39B + S0/LENGTH + 余裕) */
#define RX_BUFFER_SIZE 64

/** 送信バッファサイズ */
#define TX_BUFFER_SIZE 64

/** ADV チャネル数 */
#define ADV_CHANNEL_COUNT 3

/* ==================================================================
 * モジュール内部状態
 * ================================================================== */

/** 受信バッファ (RADIO の DMA がここに直接書き込む) */
static uint8_t s_rxBuffer[RX_BUFFER_SIZE] __attribute__((aligned(4)));

/** 現在スキャン中の ADV チャネルインデックス (0, 1, 2) */
static volatile uint8_t s_currentChannelIndex;

/** スキャン動作中フラグ */
static volatile int s_scanning;

/** 上位層への通知コールバック */
static BLEArchOnAdvertiseCallback s_onAdvertise;

/** 送信 PDU バッファ */
static uint8_t s_txBuffer[TX_BUFFER_SIZE] __attribute__((aligned(4)));
static uint8_t s_txBufferLength;

/** Advertising 動作中フラグ */
static volatile int s_advertising;

/** Advertising interval (μs) */
static uint32_t s_advertiseIntervalMicroseconds;

/* ==================================================================
 * 内部関数 (前方宣言)
 * ================================================================== */

static void ConfigureRadio(void);
static void set_radio_channel(uint8_t channel_index);
static void start_radio_receive(void);
static void disable_radio_and_wait(void);
static void ConfigureRadioForTransmit(void);
static void transmit_on_channel(uint8_t channel_index);
static void start_advertise_timer(uint32_t interval_microseconds);
static void stop_advertise_timer(void);

/* ==================================================================
 * BLEArch インターフェース実装
 * ================================================================== */

int32_t ble_arch_init(void) {
    s_scanning = 0;
    s_onAdvertise = NULL;
    ConfigureRadio();
    return 0;
}

/**
 * スキャン開始
 *
 * NOTE: 現時点では interval/window パラメータは未使用（常時スキャン）。
 * TODO: TIMER を使って interval/window 制御を実装する。
 */
int32_t ble_arch_scan_start(uint16_t interval_625us,
                            uint16_t window_625us,
                            int32_t is_passive,
                            BLEArchOnAdvertiseCallback on_advertise) {
    (void)interval_625us;
    (void)window_625us;
    (void)is_passive;

    s_onAdvertise = on_advertise;
    s_scanning = 1;
    s_currentChannelIndex = 0;

    set_radio_channel(0);
    start_radio_receive();

    NRF_RADIO->INTENSET = RADIO_INTENSET_END_Msk;

    NVIC_SetPriority(RADIO_IRQn, 6);
    NVIC_ClearPendingIRQ(RADIO_IRQn);
    NVIC_EnableIRQ(RADIO_IRQn);

    return 0;
}

int32_t ble_arch_scan_stop(void) {
    s_scanning = 0;

    NVIC_DisableIRQ(RADIO_IRQn);
    NRF_RADIO->INTENCLR = RADIO_INTENCLR_END_Msk;

    disable_radio_and_wait();

    s_onAdvertise = NULL;
    return 0;
}

int32_t ble_arch_advertise_set_pdu(const uint8_t *pdu, uint8_t pdu_length) {
    if (pdu == NULL || pdu_length > TX_BUFFER_SIZE) {
        return -1;
    }
    memcpy(s_txBuffer, pdu, pdu_length);
    s_txBufferLength = pdu_length;
    return 0;
}

/**
 * Advertising 開始
 *
 * @param interval_625us  BLE Advertising Interval (0.625ms = 625μs 単位)
 */
int32_t ble_arch_advertise_start(uint16_t interval_625us) {
    s_advertising = 1;

    /* NOTE: 0.625ms = 625μs なので interval_625us * 625 で μs に変換 */
    s_advertiseIntervalMicroseconds = (uint32_t)interval_625us * 625;

    /* 最初の Advertising Event をすぐに送信 */
    ConfigureRadioForTransmit();
    transmit_on_channel(0);
    transmit_on_channel(1);
    transmit_on_channel(2);

    start_advertise_timer(s_advertiseIntervalMicroseconds);

    return 0;
}

int32_t ble_arch_advertise_stop(void) {
    s_advertising = 0;
    stop_advertise_timer();
    disable_radio_and_wait();
    return 0;
}

/* ==================================================================
 * RADIO 割り込みハンドラ (Scanner)
 *
 * パケット受信完了 (END イベント) で発火する。
 *
 * 処理の流れ:
 *   1. CRC チェック → NG ならパケットを捨てる
 *   2. PDU ヘッダをパースしてアドレスと AD データを取り出す
 *   3. RSSI を読み取る
 *   4. コールバックで上位層に通知
 *   5. 次の ADV チャネルに切り替えて受信再開
 * ================================================================== */

void RADIO_IRQHandler(void) {
    if (NRF_RADIO->EVENTS_END == 0) {
        return;
    }
    NRF_RADIO->EVENTS_END = 0;

    /* CRC チェック: 0=NG, 1=OK */
    if (NRF_RADIO->CRCSTATUS != 1) {
        goto next_channel;
    }

    {
        /*
         * 受信バッファの解析
         *
         *   s_rxBuffer[0] = S0 (PDU Header 下位バイト)
         *     bit[3:0] = PDU Type
         *     bit[6]   = TxAdd
         *   s_rxBuffer[1] = LENGTH (payload バイト数)
         *   s_rxBuffer[2..] = Payload (AdvA 6B + AdvData 0-31B)
         */
        uint8_t pdu_header = s_rxBuffer[0];
        uint8_t pdu_length = s_rxBuffer[1];
        uint8_t pdu_type = pdu_header & 0x0F;
        uint8_t tx_add = (pdu_header >> 6) & 0x01;

        /* payload の妥当性チェック (最低6B=AdvA, 最大37B=AdvA+AdvData) */
        if (pdu_length < 6 || pdu_length > 37) {
            goto next_channel;
        }

        ble_gap_discoveryDescriptor descriptor;
        memset(&descriptor, 0, sizeof(descriptor));

        descriptor.address.type = tx_add ? 0x01 : 0x00;
        memcpy(descriptor.address.value, &s_rxBuffer[2], 6);

        uint8_t data_length = pdu_length - 6;
        descriptor.data_length = data_length;
        descriptor.data = (data_length > 0) ? &s_rxBuffer[8] : NULL;

        /* PDU Type → event_type 変換 */
        switch (pdu_type) {
        case 0: descriptor.event_type = 0; break; /* ADV_IND */
        case 1: descriptor.event_type = 1; break; /* ADV_DIRECT_IND */
        case 2: descriptor.event_type = 3; break; /* ADV_NONCONN_IND */
        case 6: descriptor.event_type = 2; break; /* ADV_SCAN_IND */
        case 4: descriptor.event_type = 4; break; /* SCAN_RSP */
        default: descriptor.event_type = 0; break;
        }

        /*
         * RSSI の取得
         *
         * NOTE: RSSISAMPLE は符号なし絶対値。例: 60 → -60dBm
         */
        descriptor.rssi = -(int8_t)NRF_RADIO->RSSISAMPLE;

        if (s_onAdvertise != NULL) {
            s_onAdvertise(&descriptor);
        }
    }

next_channel:
    if (!s_scanning) {
        return;
    }

    s_currentChannelIndex = (s_currentChannelIndex + 1) % ADV_CHANNEL_COUNT;
    set_radio_channel(s_currentChannelIndex);
    start_radio_receive();
}

/* ==================================================================
 * TIMER1 割り込みハンドラ (Advertiser)
 *
 * Advertising Interval ごとに発火し、3チャネルに ADV パケットを送信する。
 *
 * NOTE: BLE 仕様では advDelay (0-10ms ランダム遅延) を入れることが
 * 推奨されているが、簡易実装のため省略している。
 * ================================================================== */

void TIMER1_IRQHandler(void) {
    if (NRF_TIMER1->EVENTS_COMPARE[0] == 0) {
        return;
    }
    NRF_TIMER1->EVENTS_COMPARE[0] = 0;

    if (!s_advertising) {
        return;
    }

    /*
     * 3つの ADV チャネルに順次送信
     *
     * NOTE: 各チャネルの送信は ~400μs で完了する。
     * 3ch 合計 ~1.2ms は Advertising Interval (通常 100ms+) に比べて十分短い。
     */
    ConfigureRadioForTransmit();
    transmit_on_channel(0);
    transmit_on_channel(1);
    transmit_on_channel(2);
}

/* ==================================================================
 * 内部ヘルパー関数
 * ================================================================== */

/**
 * RADIO を BLE 1MBIT 受信モードに設定する
 *
 * パケットフォーマット、Access Address、CRC、ショートカット等の初期設定を行う。
 */
static void ConfigureRadio(void) {
    NRF_RADIO->MODE = RADIO_MODE_MODE_Ble_1Mbit;

    /*
     * PCNF0: PDU レイアウト
     *   LFLEN=6 (LENGTH は 6bit), S0LEN=1 (1B), S1LEN=0, PLEN=8bit
     */
    NRF_RADIO->PCNF0 = (6 << RADIO_PCNF0_LFLEN_Pos) | (1 << RADIO_PCNF0_S0LEN_Pos) | (0 << RADIO_PCNF0_S1LEN_Pos)
                       | (RADIO_PCNF0_PLEN_8bit << RADIO_PCNF0_PLEN_Pos);

    /*
     * PCNF1: ペイロード設定
     *   MAXLEN=62, BALEN=3, Little Endian, Data Whitening 有効
     */
    NRF_RADIO->PCNF1 = ((RX_BUFFER_SIZE - 2) << RADIO_PCNF1_MAXLEN_Pos) | (0 << RADIO_PCNF1_STATLEN_Pos)
                       | (3 << RADIO_PCNF1_BALEN_Pos) | (RADIO_PCNF1_ENDIAN_Little << RADIO_PCNF1_ENDIAN_Pos)
                       | (RADIO_PCNF1_WHITEEN_Enabled << RADIO_PCNF1_WHITEEN_Pos);

    /* Access Address (0x8E89BED6) を分割設定 */
    NRF_RADIO->BASE0 = (BLE_ADV_ACCESS_ADDRESS << 8) & 0xFFFFFFFF;
    NRF_RADIO->PREFIX0 = (BLE_ADV_ACCESS_ADDRESS & 0xFF);
    NRF_RADIO->RXADDRESSES = RADIO_RXADDRESSES_ADDR0_Msk;

    /* CRC: 24bit, 多項式 x^24+x^10+x^9+x^6+x^4+x^3+x+1, 初期値 0x555555 */
    NRF_RADIO->CRCCNF = (3 << RADIO_CRCCNF_LEN_Pos) | (RADIO_CRCCNF_SKIPADDR_Skip << RADIO_CRCCNF_SKIPADDR_Pos);
    NRF_RADIO->CRCPOLY = 0x00065B;
    NRF_RADIO->CRCINIT = 0x555555;

    NRF_RADIO->PACKETPTR = (uint32_t)s_rxBuffer;

    /* SHORTS: READY→START (自動受信開始), ADDRESS→RSSISTART (自動RSSI測定) */
    NRF_RADIO->SHORTS = RADIO_SHORTS_READY_START_Msk | RADIO_SHORTS_ADDRESS_RSSISTART_Msk;
}

/**
 * ADV チャネルを設定する
 * @param channel_index  0=ch37, 1=ch38, 2=ch39
 */
static void set_radio_channel(uint8_t channel_index) {
    NRF_RADIO->FREQUENCY = ADV_CHANNEL_FREQUENCY[channel_index];
    NRF_RADIO->DATAWHITEIV = ADV_CHANNEL_NUMBER[channel_index];
}

/** 受信を開始する (SHORTS READY→START で自動開始) */
static void start_radio_receive(void) {
    NRF_RADIO->PACKETPTR = (uint32_t)s_rxBuffer;
    NRF_RADIO->EVENTS_END = 0;
    NRF_RADIO->EVENTS_READY = 0;
    NRF_RADIO->EVENTS_ADDRESS = 0;
    NRF_RADIO->TASKS_RXEN = 1;
}

/** RADIO を無効化して完了を待つ */
static void disable_radio_and_wait(void) {
    NRF_RADIO->EVENTS_DISABLED = 0;
    NRF_RADIO->TASKS_DISABLE = 1;
    while (NRF_RADIO->EVENTS_DISABLED == 0) {
        /* busy wait (通常は数μs) */
    }
    NRF_RADIO->EVENTS_DISABLED = 0;
}

/**
 * RADIO を TX モードに設定する
 *
 * SHORTS を TX 用に変更し、送信バッファを指す。
 */
static void ConfigureRadioForTransmit(void) {
    NRF_RADIO->SHORTS = RADIO_SHORTS_READY_START_Msk | RADIO_SHORTS_END_DISABLE_Msk;
    NRF_RADIO->TXADDRESS = 0;
    NRF_RADIO->PACKETPTR = (uint32_t)s_txBuffer;
}

/**
 * 指定チャネルで1パケット送信して完了を待つ (同期送信)
 * @param channel_index  0=ch37, 1=ch38, 2=ch39
 */
static void transmit_on_channel(uint8_t channel_index) {
    set_radio_channel(channel_index);

    NRF_RADIO->EVENTS_READY = 0;
    NRF_RADIO->EVENTS_END = 0;
    NRF_RADIO->EVENTS_DISABLED = 0;
    NRF_RADIO->TASKS_TXEN = 1;

    /* NOTE: DISABLED を待つ = 1パケット送信完了 (通常 ~400μs) */
    while (NRF_RADIO->EVENTS_DISABLED == 0) {
        /* busy wait */
    }
    NRF_RADIO->EVENTS_DISABLED = 0;
}

/**
 * TIMER1 で Advertising Interval タイマーを開始する
 *
 * 1MHz (1μs分解能) で動作する 32bit タイマーを使用する。
 */
static void start_advertise_timer(uint32_t interval_microseconds) {
    NRF_TIMER1->TASKS_STOP = 1;
    NRF_TIMER1->TASKS_CLEAR = 1;

    NRF_TIMER1->MODE = TIMER_MODE_MODE_Timer;
    NRF_TIMER1->BITMODE = TIMER_BITMODE_BITMODE_32Bit;
    NRF_TIMER1->PRESCALER = 4; /* NOTE: 16MHz / 2^4 = 1MHz */

    NRF_TIMER1->CC[0] = interval_microseconds;
    NRF_TIMER1->SHORTS = TIMER_SHORTS_COMPARE0_CLEAR_Msk;
    NRF_TIMER1->INTENSET = TIMER_INTENSET_COMPARE0_Msk;

    NVIC_SetPriority(TIMER1_IRQn, 6);
    NVIC_ClearPendingIRQ(TIMER1_IRQn);
    NVIC_EnableIRQ(TIMER1_IRQn);

    NRF_TIMER1->TASKS_START = 1;
}

/** Advertising タイマーを停止する */
static void stop_advertise_timer(void) {
    NRF_TIMER1->TASKS_STOP = 1;
    NRF_TIMER1->INTENCLR = TIMER_INTENCLR_COMPARE0_Msk;
    NVIC_DisableIRQ(TIMER1_IRQn);
}
