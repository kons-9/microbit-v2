/**
 * @file main.cpp
 * @brief OTA Updater アプリケーション
 *
 * 最小構成の BLE DFU レシーバ。
 * BLE GATT 経由でファームウェアイメージを受信し、
 * アプリケーションスロットに書き込む。
 */

#include <utkernel/task>
#if USE_TMONITOR
#include <tm/tmonitor.h>
#endif

#include "ble.h"
#include "ota.h"

/* ==================================================================
 * Updater Task
 * ================================================================== */

static void updater_task(void *) {

    auto err = ble::init();
    if (err != 0) {
        goto fail;
    }

    err = ota::start_receive();
    if (err != 0) {
        goto fail;
    }

    ota::switch_mode(SYSCONFIG_BOOT_APP);

fail:
    utkernel::task::sleep_forever();
}

/* ==================================================================
 * Entry Point
 * ================================================================== */

static utkernel::task s_updater_task;

extern "C" int usermain(void) {
    utkernel::task::config task_config;
    task_config.priority = 10;
    task_config.stack_size = 2048;
    if (!s_updater_task.create(updater_task, task_config) || !s_updater_task.start()) {
        return 1;
    }

    utkernel::task::sleep_forever();
    return 0;
}
