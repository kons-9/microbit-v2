#include <tk/tkernel.h>
#include <tm/tmonitor.h>

#include "ble.h"
#include "ota.h"

/**
 * OTA Updater Application
 *
 * 最小構成のBLE DFUレシーバ。
 * BLE GATT経由でファームウェアイメージを受信し、
 * アプリケーションスロットに書き込む。
 */

static void updater_task(INT stacd, void *exinf);

/* ---- Updater Task ---- */

static ID tskid_updater;

static void updater_task(INT stacd, void *exinf) {
    (void)stacd;
    (void)exinf;

    tm_putstring((UB *)"[updater] Starting OTA updater...\n");

    /* BLE初期化 */
    int err = ble_init();
    if (err != 0) {
        tm_printf((UB *)"[updater] ble_init failed: %d\n", err);
        goto fail;
    }

    /* OTA受信開始 (ブロッキング) */
    err = ota_start_receive();
    if (err != 0) {
        tm_printf((UB *)"[updater] OTA receive failed: %d\n", err);
        goto fail;
    }

    tm_putstring((UB *)"[updater] OTA complete. Rebooting into app...\n");
    ota_switch_mode(SYSCONFIG_BOOT_APP);

fail:
    tm_putstring((UB *)"[updater] Fatal error. Halting.\n");
    tk_slp_tsk(TMO_FEVR);
}

/* ---- Entry Point ---- */

extern "C" EXPORT INT usermain(void) {
    T_CTSK ctsk = {};
    ctsk.tskatr = TA_HLNG | TA_RNG3;
    ctsk.task = (FP)updater_task;
    ctsk.stksz = 2048;
    ctsk.itskpri = 10;

    tskid_updater = tk_cre_tsk(&ctsk);
    if (tskid_updater < E_OK) {
        tm_printf((UB *)"[updater] tk_cre_tsk failed: %d\n", tskid_updater);
        return 1;
    }

    tk_sta_tsk(tskid_updater, 0);
    tk_slp_tsk(TMO_FEVR);
    return 0;
}
