/**
 * @file main.cpp
 * @brief OTA Updater アプリケーション
 *
 * 最小構成の BLE DFU レシーバ。
 * BLE GATT 経由でファームウェアイメージを受信し、
 * アプリケーションスロットに書き込む。
 */

#include <tk/tkernel.h>
#if USE_TMONITOR
#include <tm/tmonitor.h>
#endif

#include "ble.h"
#include "ota.h"

/* ==================================================================
 * Updater Task
 * ================================================================== */

static void updater_task(INT stacd, void *exinf) {
    (void)stacd;
    (void)exinf;

    auto err = ble_init();
    if (err != 0) {
        goto fail;
    }

    err = ota_start_receive();
    if (err != 0) {
        goto fail;
    }

    ota_switch_mode(SYSCONFIG_BOOT_APP);

fail:
    tk_slp_tsk(TMO_FEVR);
}

/* ==================================================================
 * Entry Point
 * ================================================================== */

static ID s_updaterTaskId;

extern "C" EXPORT INT usermain(void) {
    T_CTSK ctsk = {};
    ctsk.tskatr = TA_HLNG | TA_RNG3;
    ctsk.task = reinterpret_cast<FP>(updater_task);
    ctsk.stksz = 2048;
    ctsk.itskpri = 10;

    s_updaterTaskId = tk_cre_tsk(&ctsk);
    if (s_updaterTaskId < E_OK) {
        return 1;
    }

    tk_sta_tsk(s_updaterTaskId, 0);
    tk_slp_tsk(TMO_FEVR);
    return 0;
}
