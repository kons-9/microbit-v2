#pragma once

/**
 * @file ble.h
 * @brief BLE Interface (NimBLE互換サブセット)
 *
 * NimBLE の ble_gap_disc / ble_gap_adv 系 API に準拠したインターフェース。
 * - Observer (passive scan): 位置推定の受信側
 * - Broadcaster (advertising): ビーコン発信側
 *
 * @pre ble::init() を呼んでからスキャン / アドバタイズを開始すること
 */

#include <cstdint>
#include <cstddef>

namespace ble {

/* ==================================================================
 * Address
 * ================================================================== */

/** BLE アドレス長 (bytes) */
constexpr uint8_t ADDRESS_LENGTH = 6;

/** BLE アドレスタイプ */
enum class AddressType : uint8_t {
    Public = 0x00,
    Random = 0x01,
    PublicId = 0x02,
    RandomId = 0x03,
};

/** BLE アドレス */
struct Address {
    uint8_t type;
    uint8_t value[6];
};

/* ==================================================================
 * Error Codes
 * ================================================================== */

enum class Error : int32_t {
    Success = 0,
    Unknown = 1,
    InvalidParam = 2,
    Busy = 3,
    Hardware = 4,
};

/* ==================================================================
 * Scan (Discovery) Parameters
 * ================================================================== */

/** スキャンパラメータ */
struct DiscoveryParams {
    uint16_t interval;         /**< スキャン間隔 (単位: 0.625ms) */
    uint16_t window;           /**< スキャンウィンドウ (単位: 0.625ms) */
    uint8_t filter_policy;     /**< 0: accept all, 1: whitelist only */
    uint8_t is_limited;        /**< limited discovery (0 or 1) */
    uint8_t is_passive;        /**< 1=passive, 0=active */
    uint8_t filter_duplicates; /**< 重複フィルタ (0 or 1) */
};

/* ==================================================================
 * Discovery Event Descriptor
 * ================================================================== */

/** スキャン結果1件 */
struct DiscoveryDescriptor {
    Address address;     /**< Advertiser address */
    int8_t rssi;         /**< RSSI (dBm) */
    uint8_t data_length; /**< AD data length */
    const uint8_t *data; /**< AD data pointer (コールバック内のみ有効) */
    int8_t event_type;   /**< ADV_IND=0, ADV_DIRECT=1, ADV_SCAN=2, ADV_NONCONN=3, SCAN_RSP=4 */
};

/* ==================================================================
 * GAP Events
 * ================================================================== */

/** GAP イベントタイプ */
enum class GapEventType : uint8_t {
    Discovery = 0,         /**< Advertisement received */
    DiscoveryComplete = 1, /**< Discovery finished (duration expired) */
};

/** GAP イベント */
struct GapEvent {
    uint8_t type; /**< GapEventType */
    union {
        DiscoveryDescriptor discovery;
        struct {
            int32_t reason; /**< 0=完了, other=エラー */
        } discovery_complete;
    };
};

/**
 * GAP イベントコールバック型
 * @param event  受信したイベント
 * @param argument  ユーザー指定の引数
 * @return 0: continue, 非ゼロ: スキャン停止
 */
using GapEventCallback = int32_t (*)(GapEvent *event, void *argument);

/* ==================================================================
 * Discovery (Scanner / Observer) API
 * ================================================================== */

/**
 * BLE サブシステムを初期化する
 *
 * @pre 他の BLE 関数を呼ぶ前に必ず呼ぶこと
 * @post BLE ハードウェアが受信/送信可能な状態になる
 * @return 0 on success
 */
int32_t init(void);

/**
 * スキャン (Discovery) を開始する
 *
 * @pre  init() が成功していること
 * @pre  スキャン中でないこと (gap_discovery_active() == 0)
 * @post スキャンが開始され、パケット受信のたびに callback が呼ばれる
 *
 * @param own_address_type  自局アドレス種別
 * @param duration_ms       スキャン継続時間 [ms], 0=無期限
 * @param params            スキャンパラメータ
 * @param callback          イベントコールバック
 * @param callback_argument コールバック引数
 * @return 0 on success, Error on failure
 */
int32_t gap_discover(uint8_t own_address_type,
                     int32_t duration_ms,
                     const DiscoveryParams *params,
                     GapEventCallback callback,
                     void *callback_argument);

/**
 * スキャンを中止する
 *
 * @post スキャンが停止し、disc_complete コールバックが呼ばれる
 * @return 0 on success
 */
int32_t gap_discover_cancel(void);

/**
 * スキャン中かどうかを取得する
 * @return 1=scanning, 0=idle
 */
int32_t gap_discovery_active(void);

/* ==================================================================
 * Advertise (Broadcaster) API
 * ================================================================== */

/** AD データの最大長 (BLE 4.x 仕様: 31 bytes) */
constexpr uint8_t ADVERTISE_DATA_MAX_LENGTH = 31;

/** Advertising パラメータ */
struct GapAdvertiseParams {
    /**
     * Advertising 間隔 (単位: 0.625ms)
     *
     * BLE 仕様では 20ms (=32) 〜 10.24s (=16384) の範囲。
     * 例: 160 = 100ms, 1600 = 1000ms
     */
    uint16_t interval_min;
    uint16_t interval_max;

    /**
     * ADV PDU タイプ
     *   0 = ADV_IND          (connectable undirected)
     *   2 = ADV_NONCONN_IND  (non-connectable undirected) ← ビーコン用
     *   6 = ADV_SCAN_IND     (scannable undirected)
     */
    uint8_t advertise_type;
};

/**
 * Advertising データを設定する
 *
 * @pre  init() が成功していること
 * @post データが内部バッファにコピーされ、次回 gap_advertise_start() で使用される
 *
 * @param data  AD 構造体の配列 (AD Length + AD Type + AD Data の繰り返し)
 * @param length データ長 (最大 ADVERTISE_DATA_MAX_LENGTH)
 * @return 0 on success
 */
int32_t gap_advertise_set_data(const uint8_t *data, uint8_t length);

/**
 * Advertising を開始する
 *
 * @pre  init() が成功していること
 * @pre  gap_advertise_set_data() でデータが設定済みであること
 * @post 指定間隔で ch37/38/39 に ADV パケットが送信される
 *
 * @param own_address_type  自局アドレス種別
 * @param params            Advertising パラメータ
 * @return 0 on success, Error on failure
 */
int32_t gap_advertise_start(uint8_t own_address_type, const GapAdvertiseParams *params);

/**
 * Advertising を停止する
 *
 * @post Advertising が停止する
 * @return 0 on success
 */
int32_t gap_advertise_stop(void);

/**
 * Advertising 中かどうかを取得する
 * @return 1=advertising, 0=idle
 */
int32_t gap_advertise_active(void);

}  // namespace ble
