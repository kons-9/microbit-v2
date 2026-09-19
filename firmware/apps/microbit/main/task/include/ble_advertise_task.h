#pragma once

/**
 * @file ble_advertise_task.h
 * @brief メインアプリケーションのBLE Advertisingタスク
 */

#include "ble.h"

#include <utkernel/task>

namespace app::task {

/**
 * @brief BLE Advertisingを開始し、状態を監視するタスク
 *
 * 現時点では起動時に固定のAdvertisingデータを設定し、
 * Advertisingを開始した後はデバッグ状態を定期的にログ出力する。
 */
class BleAdvertiseTask {
  public:
    static BleAdvertiseTask &instance();

    BleAdvertiseTask(const BleAdvertiseTask &) = delete;
    BleAdvertiseTask &operator=(const BleAdvertiseTask &) = delete;

    /** タスクを生成して起動する */
    bool start();

  private:
    BleAdvertiseTask() = default;

    static void entry(void *argument);
    void run();

    static BleAdvertiseTask s_instance;
    drivers::Ble m_ble;
    utkernel::task m_task;
};

}  // namespace app::task
