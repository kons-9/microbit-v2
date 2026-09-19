/**
 * @file ble_microbit.cpp
 * @brief BLE Observer/Broadcaster — nRF52833 RADIO 直叩き実装
 *
 * BLE の Advertising パケットの受信 (Observer) と送信 (Broadcaster) を
 * nRF52833 の RADIO ペリフェラルを直接操作して実現する。
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
#include <utkernel/interrupt>

#include <cstring>

/* ==================================================================
 * Configuration
 * ================================================================== */

/*
 * BLEの通信仕様、RADIOレジスタ、タイミングを論理単位で分ける。
 * これらは実行中に変化しないためconstexprとし、InnerStateとは分離する。
 */
struct BleConfig {
    struct Packet {
        static constexpr uint32_t RxBufferSize = 64;
        static constexpr uint32_t TxBufferSize = 64;
        static constexpr uint32_t MaxPayloadLength = 37;
    };

    struct Advertising {
        static constexpr uint32_t AccessAddress = 0x8E89BED6U;
        static constexpr uint32_t ChannelCount = 3;

        /* FREQUENCY: 2400MHz + value (ch37=2402, ch38=2426, ch39=2480) */
        static constexpr uint8_t ChannelFrequency[] = {2, 26, 80};

        /* DATAWHITEIV: channel number (hardware readback includes bit 6). */
        static constexpr uint8_t ChannelNumber[] = {37, 38, 39};
    };

    struct Radio {
        static constexpr uint32_t Pcnf0 = (6U << RADIO_PCNF0_LFLEN_Pos) | (1U << RADIO_PCNF0_S0LEN_Pos)
                                          | (2U << RADIO_PCNF0_S1LEN_Pos)
                                          | (RADIO_PCNF0_PLEN_8bit << RADIO_PCNF0_PLEN_Pos);
        static constexpr uint32_t Pcnf1 = (37U << RADIO_PCNF1_MAXLEN_Pos) | (3U << RADIO_PCNF1_BALEN_Pos)
                                          | (RADIO_PCNF1_ENDIAN_Little << RADIO_PCNF1_ENDIAN_Pos)
                                          | (RADIO_PCNF1_WHITEEN_Enabled << RADIO_PCNF1_WHITEEN_Pos);
        static constexpr uint32_t CrcCnf
            = (3U << RADIO_CRCCNF_LEN_Pos) | (RADIO_CRCCNF_SKIPADDR_Skip << RADIO_CRCCNF_SKIPADDR_Pos);
        static constexpr uint32_t CrcPoly = 0x00065BU;
        static constexpr uint32_t CrcInit = 0x00555555U;
        static constexpr uint32_t TifsUs = 150;
        static constexpr uint32_t RxShorts = RADIO_SHORTS_READY_START_Msk | RADIO_SHORTS_ADDRESS_RSSISTART_Msk;
        static constexpr uint32_t TxShorts = RADIO_SHORTS_READY_START_Msk | RADIO_SHORTS_END_DISABLE_Msk;
        static constexpr uint32_t DataWhiteIvReadbackBit = 1U << 6;
    };

    struct Timing {
        static constexpr uint32_t TimerPrescaler = 4;
        static constexpr uint32_t InterChannelDelayUs = 150;
        static constexpr uint32_t TxWaitLimit = 1000000;
    };
};

/* ==================================================================
 * モジュール内部状態
 * ================================================================== */

/*
 * RADIOのDMAバッファ、割り込みから更新される状態、Advertisingの
 * デバッグ情報は、スキャン/送信の処理をまたいで保持する必要がある。
 * これらは公開APIの状態ではなく、ハードウェア制御のためだけに必要な
 * 実装内部状態なのでInnerStateにまとめ、管理対象を明示する。
 */
struct ScannerState {
    /** 受信バッファ (RADIO の DMA がここに直接書き込む) */
    uint8_t rxBuffer[BleConfig::Packet::RxBufferSize] __attribute__((aligned(4)));
    volatile uint8_t currentChannelIndex;
    volatile int active;
    BLEArchOnAdvertiseCallback callback;
    void *callbackArgument;
};

struct AdvertiserState {
    /** 送信 PDU バッファ */
    uint8_t txBuffer[BleConfig::Packet::TxBufferSize] __attribute__((aligned(4)));
    uint8_t txBufferLength;
    volatile int active;
};

struct DebugState {
    struct Counters {
        volatile uint32_t timerInterruptCount;
        volatile uint32_t advertiseEventCount;
        volatile uint32_t txAttemptCount;
        volatile uint32_t txCompleteCount;
        volatile uint32_t txTimeoutCount;
        volatile uint32_t txErrorCount;
    } counters;

    struct RadioSnapshot {
        volatile uint32_t lastChannel;
        volatile uint32_t lastFrequency;
        volatile uint32_t lastRadioState;
        volatile uint32_t lastRadioEvents;
        volatile uint32_t lastCrcStatus;
    } radio;
};

struct TimerState {
    uint32_t advertiseIntervalMicroseconds;
    utkernel::interrupt timer1Interrupt;
};

struct InnerState {
    ScannerState scanner;
    AdvertiserState advertiser;
    DebugState debug;
    TimerState timer;
};

static InnerState s_state{};

/* ==================================================================
 * 内部関数 (前方宣言)
 * ================================================================== */

static void ConfigureRadio(void);
static void set_radio_channel(uint8_t channel_index);
static void start_radio_receive(void);
static void disable_radio_and_wait(void);
static void ConfigureRadioForTransmit(void);
static bool transmit_on_channel(uint8_t channel_index);
static bool transmit_advertising_event(void);
static void delay_microseconds(uint32_t microseconds);
static void start_advertise_timer(uint32_t interval_microseconds);
static void stop_advertise_timer(void);
static void advertise_timer_isr(uint32_t intno);
static void snapshot_radio_status(void);
static uint32_t radio_config_mismatch(void);

static inline void increment_counter(volatile uint32_t *counter) {
    uint32_t value = *counter;
    *counter = value + 1U;
}

/* ==================================================================
 * BLEArch インターフェース実装
 * ================================================================== */

int32_t ble_arch_init(void) {
    s_state.scanner.active = 0;
    s_state.scanner.callback = NULL;
    s_state.scanner.callbackArgument = NULL;

    if (!s_state.timer.timer1Interrupt.define(TIMER1_IRQn, advertise_timer_isr)) {
        return -1;
    }

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
                            BLEArchOnAdvertiseCallback on_advertise,
                            void *argument) {
    (void)interval_625us;
    (void)window_625us;
    (void)is_passive;

    s_state.scanner.callback = on_advertise;
    s_state.scanner.callbackArgument = argument;
    s_state.scanner.active = 1;
    s_state.scanner.currentChannelIndex = 0;

    set_radio_channel(0);
    start_radio_receive();

    NRF_RADIO->INTENSET = RADIO_INTENSET_END_Msk;

    NVIC_SetPriority(RADIO_IRQn, 6);
    NVIC_ClearPendingIRQ(RADIO_IRQn);
    NVIC_EnableIRQ(RADIO_IRQn);

    return 0;
}

int32_t ble_arch_scan_stop(void) {
    s_state.scanner.active = 0;

    NVIC_DisableIRQ(RADIO_IRQn);
    NRF_RADIO->INTENCLR = RADIO_INTENCLR_END_Msk;

    disable_radio_and_wait();

    s_state.scanner.callback = NULL;
    s_state.scanner.callbackArgument = NULL;
    return 0;
}

int32_t ble_arch_advertise_set_pdu(const uint8_t *pdu, uint8_t pdu_length) {
    if (pdu == NULL || pdu_length > BleConfig::Packet::TxBufferSize) {
        return -1;
    }
    memcpy(s_state.advertiser.txBuffer, pdu, pdu_length);
    s_state.advertiser.txBufferLength = pdu_length;
    return 0;
}

int32_t ble_arch_get_advertise_debug_status(ble::AdvertiseDebugStatus *status) {
    if (status == NULL) {
        return -1;
    }

    status->timer_interrupt_count = s_state.debug.counters.timerInterruptCount;
    status->advertise_event_count = s_state.debug.counters.advertiseEventCount;
    status->tx_attempt_count = s_state.debug.counters.txAttemptCount;
    status->tx_complete_count = s_state.debug.counters.txCompleteCount;
    status->tx_timeout_count = s_state.debug.counters.txTimeoutCount;
    status->tx_error_count = s_state.debug.counters.txErrorCount;
    status->pdu_length = s_state.advertiser.txBufferLength;
    status->timer_compare = NRF_TIMER1->CC[0];
    status->last_channel = s_state.debug.radio.lastChannel;
    status->last_frequency = s_state.debug.radio.lastFrequency;
    status->last_radio_state = s_state.debug.radio.lastRadioState;
    status->last_radio_events = s_state.debug.radio.lastRadioEvents;
    status->last_crcstatus = s_state.debug.radio.lastCrcStatus;

    status->radio_power = NRF_RADIO->POWER;
    status->radio_frequency = NRF_RADIO->FREQUENCY;
    status->radio_mode = NRF_RADIO->MODE;
    status->radio_pcnf0 = NRF_RADIO->PCNF0;
    status->radio_pcnf1 = NRF_RADIO->PCNF1;
    status->radio_base0 = NRF_RADIO->BASE0;
    status->radio_prefix0 = NRF_RADIO->PREFIX0;
    status->radio_crccnf = NRF_RADIO->CRCCNF;
    status->radio_crcpoly = NRF_RADIO->CRCPOLY;
    status->radio_crcinit = NRF_RADIO->CRCINIT;
    status->radio_txpower = NRF_RADIO->TXPOWER;
    status->radio_tifs = NRF_RADIO->TIFS;
    status->radio_shorts = NRF_RADIO->SHORTS;
    status->radio_packetptr = NRF_RADIO->PACKETPTR;
    status->radio_txaddress = NRF_RADIO->TXADDRESS;
    status->radio_rxaddresses = NRF_RADIO->RXADDRESSES;
    status->radio_datawhiteiv = NRF_RADIO->DATAWHITEIV;
    status->radio_config_mismatch = radio_config_mismatch();

    /* FICRは工場書き込み済みのread-only情報。RADIO設定とは独立したHW確認用。 */
    status->ficr_part = NRF_FICR->INFO.PART;
    status->ficr_variant = NRF_FICR->INFO.VARIANT;
    status->ficr_package = NRF_FICR->INFO.PACKAGE;
    status->ficr_ram = NRF_FICR->INFO.RAM;
    status->ficr_flash = NRF_FICR->INFO.FLASH;
    status->ficr_deviceid0 = NRF_FICR->DEVICEID[0];
    status->ficr_deviceid1 = NRF_FICR->DEVICEID[1];
    return 0;
}

/**
 * Advertising 開始
 *
 * @param interval_625us  BLE Advertising Interval (0.625ms = 625μs 単位)
 */
int32_t ble_arch_advertise_start(uint16_t interval_625us) {
    s_state.debug.counters.timerInterruptCount = 0;
    s_state.debug.counters.advertiseEventCount = 0;
    s_state.debug.counters.txAttemptCount = 0;
    s_state.debug.counters.txCompleteCount = 0;
    s_state.debug.counters.txTimeoutCount = 0;
    s_state.debug.counters.txErrorCount = 0;

    s_state.advertiser.active = 1;

    /* NOTE: 0.625ms = 625μs なので interval_625us * 625 で μs に変換 */
    s_state.timer.advertiseIntervalMicroseconds = (uint32_t)interval_625us * 625;

    /* 最初の Advertising Event をすぐに送信 */
    ConfigureRadioForTransmit();
    increment_counter(&s_state.debug.counters.advertiseEventCount);
    if (!transmit_advertising_event()) {
        increment_counter(&s_state.debug.counters.txErrorCount);
    }

    start_advertise_timer(s_state.timer.advertiseIntervalMicroseconds);

    return 0;
}

int32_t ble_arch_advertise_stop(void) {
    s_state.advertiser.active = 0;
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

extern "C" void RADIO_IRQHandler(void) {
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
         *   s_state.scanner.rxBuffer[0] = S0 (PDU Header 下位バイト)
         *     bit[3:0] = PDU Type
         *     bit[6]   = TxAdd
         *   s_state.scanner.rxBuffer[1] = LENGTH (payload バイト数)
         *   s_state.scanner.rxBuffer[2] = S1 (RAM上の1byte。on-airでは2bit)
         *   s_state.scanner.rxBuffer[3..] = Payload (AdvA 6B + AdvData 0-31B)
         */
        uint8_t pdu_header = s_state.scanner.rxBuffer[0];
        uint8_t pdu_length = s_state.scanner.rxBuffer[1];
        uint8_t pdu_type = pdu_header & 0x0F;
        uint8_t tx_add = (pdu_header >> 6) & 0x01;

        /* payload の妥当性チェック (最低6B=AdvA, 最大37B=AdvA+AdvData) */
        if (pdu_length < 6 || pdu_length > BleConfig::Packet::MaxPayloadLength) {
            goto next_channel;
        }

        ble::DiscoveryDescriptor descriptor;
        memset(&descriptor, 0, sizeof(descriptor));

        descriptor.address.type = tx_add ? ble::AddressType::Random : ble::AddressType::Public;
        memcpy(descriptor.address.value, &s_state.scanner.rxBuffer[3], 6);

        uint8_t data_length = pdu_length - 6;
        descriptor.data_length = data_length;
        descriptor.data = (data_length > 0) ? &s_state.scanner.rxBuffer[9] : NULL;

        /* PDU Type → event_type 変換 */
        switch (pdu_type) {
        case 0: descriptor.event_type = ble::AdvertiseEventType::ConnectableUndirected; break;
        case 1: descriptor.event_type = ble::AdvertiseEventType::ConnectableDirected; break;
        case 2: descriptor.event_type = ble::AdvertiseEventType::NonConnectableUndirected; break;
        case 6: descriptor.event_type = ble::AdvertiseEventType::ScannableUndirected; break;
        case 4: descriptor.event_type = ble::AdvertiseEventType::ScanResponse; break;
        default: descriptor.event_type = ble::AdvertiseEventType::ConnectableUndirected; break;
        }

        /*
         * RSSI の取得
         *
         * NOTE: RSSISAMPLE は符号なし絶対値。例: 60 → -60dBm
         */
        descriptor.rssi = -(int8_t)NRF_RADIO->RSSISAMPLE;

        if (s_state.scanner.callback != NULL) {
            s_state.scanner.callback(&descriptor, s_state.scanner.callbackArgument);
        }
    }

next_channel:
    if (!s_state.scanner.active) {
        return;
    }

    s_state.scanner.currentChannelIndex
        = (s_state.scanner.currentChannelIndex + 1) % BleConfig::Advertising::ChannelCount;
    set_radio_channel(s_state.scanner.currentChannelIndex);
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

static void advertise_timer_isr(uint32_t intno) {
    (void)intno;

    if (NRF_TIMER1->EVENTS_COMPARE[0] == 0) {
        return;
    }
    NRF_TIMER1->EVENTS_COMPARE[0] = 0;

    increment_counter(&s_state.debug.counters.timerInterruptCount);

    if (!s_state.advertiser.active) {
        return;
    }

    /*
     * 3つの ADV チャネルに順次送信
     *
     * NOTE: 各チャネルの送信は ~400μs で完了する。
     * 3ch 合計 ~1.2ms は Advertising Interval (通常 100ms+) に比べて十分短い。
     */
    ConfigureRadioForTransmit();
    increment_counter(&s_state.debug.counters.advertiseEventCount);
    if (!transmit_advertising_event()) {
        increment_counter(&s_state.debug.counters.txErrorCount);
    }
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
    NRF_RADIO->POWER = 1;
    NRF_RADIO->MODE = RADIO_MODE_MODE_Ble_1Mbit;
    /* 切り分け用に最大出力(+8dBm)。通常運用では出力を下げる。 */
    NRF_RADIO->TXPOWER = RADIO_TXPOWER_TXPOWER_Pos8dBm;

    /*
     * PCNF0: PDU レイアウト
     *   LFLEN=6 (LENGTH は 6bit), S0LEN=1 (1B), S1LEN=2 (予約ビット), PLEN=8bit
     */
    NRF_RADIO->PCNF0 = BleConfig::Radio::Pcnf0;

    /*
     * PCNF1: ペイロード設定
     *   MAXLEN=37, BALEN=3, Little Endian, Data Whitening 有効
     */
    NRF_RADIO->PCNF1 = BleConfig::Radio::Pcnf1;

    /* Access Address (0x8E89BED6) を分割設定 */
    NRF_RADIO->BASE0 = BleConfig::Advertising::AccessAddress << 8;
    NRF_RADIO->PREFIX0 = BleConfig::Advertising::AccessAddress >> 24;
    NRF_RADIO->RXADDRESSES = RADIO_RXADDRESSES_ADDR0_Msk;

    /* CRC: 24bit, 多項式 x^24+x^10+x^9+x^6+x^4+x^3+x+1, 初期値 0x555555 */
    NRF_RADIO->CRCCNF = BleConfig::Radio::CrcCnf;
    NRF_RADIO->CRCPOLY = BleConfig::Radio::CrcPoly;
    NRF_RADIO->CRCINIT = BleConfig::Radio::CrcInit;
    NRF_RADIO->TIFS = BleConfig::Radio::TifsUs;

    NRF_RADIO->PACKETPTR = (uint32_t)s_state.scanner.rxBuffer;

    /* SHORTS: READY→START (自動受信開始), ADDRESS→RSSISTART (自動RSSI測定) */
    NRF_RADIO->SHORTS = BleConfig::Radio::RxShorts;
}

/**
 * ADV チャネルを設定する
 * @param channel_index  0=ch37, 1=ch38, 2=ch39
 */
static void set_radio_channel(uint8_t channel_index) {
    NRF_RADIO->FREQUENCY = BleConfig::Advertising::ChannelFrequency[channel_index];
    NRF_RADIO->DATAWHITEIV = BleConfig::Advertising::ChannelNumber[channel_index];
}

/** 受信を開始する (SHORTS READY→START で自動開始) */
static void start_radio_receive(void) {
    NRF_RADIO->PACKETPTR = (uint32_t)s_state.scanner.rxBuffer;
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
    NRF_RADIO->SHORTS = BleConfig::Radio::TxShorts;
    NRF_RADIO->TXADDRESS = 0;
    NRF_RADIO->PACKETPTR = (uint32_t)s_state.advertiser.txBuffer;
}

/**
 * RADIO の主要な設定レジスタを読み戻し、期待値との差分をビットで返す。
 *
 * これは送信成否ではなく、CPUからRADIOレジスタへの書き込みが実際に
 * 反映されているかを確認するための診断用チェックである。
 */
static uint32_t radio_config_mismatch(void) {
    uint32_t mismatch = 0;
    if (NRF_RADIO->POWER != 1U)
        mismatch |= (1U << 0);
    if (NRF_RADIO->MODE != RADIO_MODE_MODE_Ble_1Mbit)
        mismatch |= (1U << 1);
    if (NRF_RADIO->TXPOWER != RADIO_TXPOWER_TXPOWER_Pos8dBm)
        mismatch |= (1U << 2);
    if (NRF_RADIO->PCNF0 != BleConfig::Radio::Pcnf0)
        mismatch |= (1U << 3);
    if (NRF_RADIO->PCNF1 != BleConfig::Radio::Pcnf1)
        mismatch |= (1U << 4);
    if (NRF_RADIO->BASE0 != (BleConfig::Advertising::AccessAddress << 8))
        mismatch |= (1U << 5);
    if (NRF_RADIO->PREFIX0 != (BleConfig::Advertising::AccessAddress >> 24))
        mismatch |= (1U << 6);
    if (NRF_RADIO->RXADDRESSES != RADIO_RXADDRESSES_ADDR0_Msk)
        mismatch |= (1U << 7);
    if (NRF_RADIO->CRCCNF != BleConfig::Radio::CrcCnf)
        mismatch |= (1U << 8);
    if (NRF_RADIO->CRCPOLY != BleConfig::Radio::CrcPoly)
        mismatch |= (1U << 9);
    if (NRF_RADIO->CRCINIT != BleConfig::Radio::CrcInit)
        mismatch |= (1U << 10);
    if (NRF_RADIO->TIFS != BleConfig::Radio::TifsUs)
        mismatch |= (1U << 11);

    if (s_state.advertiser.active) {
        if (NRF_RADIO->SHORTS != BleConfig::Radio::TxShorts)
            mismatch |= (1U << 12);
        if (NRF_RADIO->TXADDRESS != 0U)
            mismatch |= (1U << 13);
        if (NRF_RADIO->PACKETPTR != (uint32_t)s_state.advertiser.txBuffer)
            mismatch |= (1U << 14);

        const uint8_t channel = (s_state.debug.radio.lastChannel < BleConfig::Advertising::ChannelCount)
                                    ? static_cast<uint8_t>(s_state.debug.radio.lastChannel)
                                    : 0;
        if (NRF_RADIO->FREQUENCY != BleConfig::Advertising::ChannelFrequency[channel])
            mismatch |= (1U << 15);
        /* DATAWHITEIV bit 6 is read back as 1 by the nRF52 hardware. */
        const uint32_t expected_white_iv
            = BleConfig::Advertising::ChannelNumber[channel] | BleConfig::Radio::DataWhiteIvReadbackBit;
        if (NRF_RADIO->DATAWHITEIV != expected_white_iv)
            mismatch |= (1U << 16);
    }

    return mismatch;
}

/**
 * 指定チャネルで1パケット送信して完了を待つ (同期送信)
 * @param channel_index  0=ch37, 1=ch38, 2=ch39
 */
static bool transmit_on_channel(uint8_t channel_index) {
    set_radio_channel(channel_index);
    s_state.debug.radio.lastChannel = channel_index;
    s_state.debug.radio.lastFrequency = NRF_RADIO->FREQUENCY;
    increment_counter(&s_state.debug.counters.txAttemptCount);

    /* 前回送信のイベントを残さず、今回の送信結果だけを記録する。 */
    NRF_RADIO->EVENTS_READY = 0;
    NRF_RADIO->EVENTS_ADDRESS = 0;
    NRF_RADIO->EVENTS_END = 0;
    NRF_RADIO->EVENTS_PAYLOAD = 0;
    NRF_RADIO->EVENTS_DISABLED = 0;
    NRF_RADIO->TASKS_TXEN = 1;

    /* NOTE: DISABLED を待つ = 1パケット送信完了 (通常 ~400μs) */
    uint32_t wait_count = 0;
    while (NRF_RADIO->EVENTS_DISABLED == 0 && wait_count++ < BleConfig::Timing::TxWaitLimit) {
        /* busy wait */
    }

    if (NRF_RADIO->EVENTS_DISABLED == 0) {
        increment_counter(&s_state.debug.counters.txTimeoutCount);
        snapshot_radio_status();
        NRF_RADIO->TASKS_DISABLE = 1;
        return false;
    }

    snapshot_radio_status();
    NRF_RADIO->EVENTS_DISABLED = 0;
    increment_counter(&s_state.debug.counters.txCompleteCount);
    return true;
}

/** 送信完了直後またはタイムアウト時のRADIO状態を保存する。 */
static void snapshot_radio_status(void) {
    s_state.debug.radio.lastRadioState = NRF_RADIO->STATE;
    s_state.debug.radio.lastRadioEvents = (NRF_RADIO->EVENTS_READY ? 0x01U : 0U) | (NRF_RADIO->EVENTS_END ? 0x02U : 0U)
                                          | (NRF_RADIO->EVENTS_DISABLED ? 0x04U : 0U)
                                          | (NRF_RADIO->EVENTS_ADDRESS ? 0x08U : 0U)
                                          | (NRF_RADIO->EVENTS_PAYLOAD ? 0x10U : 0U);
    s_state.debug.radio.lastCrcStatus = NRF_RADIO->CRCSTATUS;
}

/**
 * 1つのAdvertising Eventを送信する。
 * BLE仕様のT_IFSを満たすため、チャネル間に150usの待ち時間を入れる。
 */
static bool transmit_advertising_event(void) {
    if (!transmit_on_channel(0)) {
        return false;
    }
    delay_microseconds(BleConfig::Timing::InterChannelDelayUs);

    if (!transmit_on_channel(1)) {
        return false;
    }
    delay_microseconds(BleConfig::Timing::InterChannelDelayUs);

    return transmit_on_channel(2);
}

/** TIMER0をポーリングして短い待ち時間を生成する。TIMER0はBLEで使用しない。 */
static void delay_microseconds(uint32_t microseconds) {
    NRF_TIMER0->TASKS_STOP = 1;
    NRF_TIMER0->TASKS_CLEAR = 1;
    NRF_TIMER0->MODE = TIMER_MODE_MODE_Timer;
    NRF_TIMER0->BITMODE = TIMER_BITMODE_BITMODE_16Bit;
    NRF_TIMER0->PRESCALER = BleConfig::Timing::TimerPrescaler; /* 16MHz / 2^4 = 1MHz */
    NRF_TIMER0->CC[0] = microseconds;
    NRF_TIMER0->SHORTS = 0;
    NRF_TIMER0->EVENTS_COMPARE[0] = 0;
    NRF_TIMER0->TASKS_START = 1;
    while (NRF_TIMER0->EVENTS_COMPARE[0] == 0) {
        /* busy wait */
    }
    NRF_TIMER0->TASKS_STOP = 1;
}

/**
 * TIMER1 で Advertising Interval タイマーを開始する
 *
 * 1MHz (1μs分解能) で動作する 32bit タイマーを使用する。
 */
static void start_advertise_timer(uint32_t interval_microseconds) {
    if (!s_state.timer.timer1Interrupt.defined()) {
        return;
    }

    NRF_TIMER1->TASKS_STOP = 1;
    NRF_TIMER1->TASKS_CLEAR = 1;

    NRF_TIMER1->MODE = TIMER_MODE_MODE_Timer;
    NRF_TIMER1->BITMODE = TIMER_BITMODE_BITMODE_32Bit;
    NRF_TIMER1->PRESCALER = BleConfig::Timing::TimerPrescaler; /* NOTE: 16MHz / 2^4 = 1MHz */

    NRF_TIMER1->CC[0] = interval_microseconds;
    NRF_TIMER1->SHORTS = TIMER_SHORTS_COMPARE0_CLEAR_Msk;
    NRF_TIMER1->INTENSET = TIMER_INTENSET_COMPARE0_Msk;

    ClearInt(TIMER1_IRQn);
    s_state.timer.timer1Interrupt.enable(6);

    NRF_TIMER1->TASKS_START = 1;
}

/** Advertising タイマーを停止する */
static void stop_advertise_timer(void) {
    NRF_TIMER1->TASKS_STOP = 1;
    NRF_TIMER1->INTENCLR = TIMER_INTENCLR_COMPARE0_Msk;
    s_state.timer.timer1Interrupt.disable();
    ClearInt(TIMER1_IRQn);
}
