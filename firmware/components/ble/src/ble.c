#include "ble.h"
#include "arch/ble_arch.h"

static int s_scanning;
static ble_gap_event_fn s_cb;
static void *s_cb_arg;

int ble_init(void) {
    s_scanning = 0;
    s_cb = NULL;
    s_cb_arg = NULL;
    return ble_arch_init();
}

int ble_gap_disc(uint8_t own_addr_type,
                 int32_t duration_ms,
                 const ble_gap_disc_params *params,
                 ble_gap_event_fn cb,
                 void *cb_arg) {
    (void)own_addr_type;
    (void)duration_ms;

    if (s_scanning) {
        return BLE_ERR_BUSY;
    }
    if (!cb || !params) {
        return BLE_ERR_INVALID_PARAM;
    }

    s_cb = cb;
    s_cb_arg = cb_arg;
    s_scanning = 1;

    int rc = ble_arch_scan_start(params->itvl, params->window, params->passive);
    if (rc != 0) {
        s_scanning = 0;
        return BLE_ERR_HW;
    }

    return BLE_ERR_SUCCESS;
}

int ble_gap_disc_cancel(void) {
    if (!s_scanning) {
        return BLE_ERR_SUCCESS;
    }

    ble_arch_scan_stop();
    s_scanning = 0;

    /* disc_complete イベント通知 */
    if (s_cb) {
        ble_gap_event ev = {0};
        ev.type = BLE_GAP_EVENT_DISC_COMPLETE;
        ev.disc_complete.reason = 0;
        s_cb(&ev, s_cb_arg);
    }

    return BLE_ERR_SUCCESS;
}

int ble_gap_disc_active(void) {
    return s_scanning;
}
