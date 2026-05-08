#pragma once

/**
 * BLE Scanner Interface (NimBLE-compatible subset)
 *
 * NimBLE の ble_gap_disc 系 API に準拠したインターフェース。
 * 位置推定に必要な Observer (passive scan) 部分のみ。
 */

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ---- Address ---- */

#define BLE_ADDR_LEN 6

#define BLE_ADDR_PUBLIC 0x00
#define BLE_ADDR_RANDOM 0x01
#define BLE_ADDR_PUBLIC_ID 0x02
#define BLE_ADDR_RANDOM_ID 0x03

typedef struct {
    uint8_t type;
    uint8_t val[BLE_ADDR_LEN];
} ble_addr_t;

/* ---- Scan (Discovery) Parameters ---- */

/** NimBLE互換: ble_gap_disc_params */
typedef struct {
    uint16_t itvl;         /**< スキャン間隔 (単位: 0.625ms) */
    uint16_t window;       /**< スキャンウィンドウ (単位: 0.625ms) */
    uint8_t filter_policy; /**< 0: accept all, 1: whitelist only */
    uint8_t limited : 1;   /**< limited discovery */
    uint8_t passive : 1;   /**< 1=passive, 0=active */
    uint8_t filter_duplicates : 1;
} ble_gap_disc_params;

/* ---- Discovery Event Descriptor ---- */

/** NimBLE互換: ble_gap_disc_desc (スキャン結果1件) */
typedef struct {
    ble_addr_t addr;     /**< Advertiser address */
    int8_t rssi;         /**< RSSI (dBm) */
    uint8_t length_data; /**< AD data length */
    const uint8_t *data; /**< AD data pointer (コールバック内のみ有効) */
    int8_t event_type;   /**< ADV_IND=0, ADV_DIRECT=1, ADV_SCAN=2, ADV_NONCONN=3, SCAN_RSP=4 */
} ble_gap_disc_desc;

/* ---- GAP Events ---- */

#define BLE_GAP_EVENT_DISC 0          /**< Advertisement received */
#define BLE_GAP_EVENT_DISC_COMPLETE 1 /**< Discovery finished (duration expired) */

typedef struct {
    uint8_t type; /**< BLE_GAP_EVENT_xxx */
    union {
        ble_gap_disc_desc disc; /**< BLE_GAP_EVENT_DISC */
        struct {
            int reason; /**< 0=完了, other=エラー */
        } disc_complete;
    };
} ble_gap_event;

/**
 * GAP イベントコールバック型
 * @return 0: continue, non-zero: stop scanning
 */
typedef int (*ble_gap_event_fn)(ble_gap_event *event, void *arg);

/* ---- API ---- */

/**
 * BLEサブシステム初期化
 * @return 0 on success
 */
int ble_init(void);

/**
 * スキャン (Discovery) 開始
 *
 * NimBLE互換: ble_gap_disc(own_addr_type, duration_ms, disc_params, cb, cb_arg)
 *
 * @param own_addr_type   自局アドレス種別 (BLE_ADDR_PUBLIC etc.)
 * @param duration_ms     スキャン継続時間 [ms], 0=無期限
 * @param params          スキャンパラメータ
 * @param cb              イベントコールバック
 * @param cb_arg          コールバック引数
 * @return 0 on success, BLE_ERR_xxx on failure
 */
int ble_gap_disc(uint8_t own_addr_type,
                 int32_t duration_ms,
                 const ble_gap_disc_params *params,
                 ble_gap_event_fn cb,
                 void *cb_arg);

/**
 * スキャン中止
 * @return 0 on success
 */
int ble_gap_disc_cancel(void);

/**
 * スキャン中かどうか
 * @return 1=scanning, 0=idle
 */
int ble_gap_disc_active(void);

/* ---- Error codes ---- */

#define BLE_ERR_SUCCESS 0
#define BLE_ERR_UNKNOWN 1
#define BLE_ERR_INVALID_PARAM 2
#define BLE_ERR_BUSY 3
#define BLE_ERR_HW 4

#ifdef __cplusplus
}
#endif
