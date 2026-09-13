#pragma once

/**
 * @file entry_task.h
 * @brief メインアプリケーションのエントリータスク
 */

#include "ble_advertise_task.h"
#include "button.h"
#include "flash.h"
#include "fs.h"
#include "uart.h"

#include <utkernel/task>

namespace app {

/** アプリケーションで共有するドライバ群 */
struct Drivers {
    drivers::Uart uart;
    drivers::Flash flash;
    drivers::Button button;
};

/** アプリケーション設定と依存関係 */
struct Config {
    Drivers &drivers;
    fs::FileSystem file_system;
    fs::RingBufferFile log_file;

    explicit Config(Drivers &drivers)
        : drivers(drivers)
        , file_system(drivers.flash)
        , log_file(file_system, fs::FileId::Log) {
    }
};

namespace task {

/**
 * @brief アプリケーションのエントリータスク
 *
 * UART、ボタン、FSを初期化し、アプリケーション配下のタスクを起動する。
 */
class EntryTask {
  public:
    static EntryTask &instance();

    EntryTask(const EntryTask &) = delete;
    EntryTask &operator=(const EntryTask &) = delete;

    /** エントリータスクを生成して起動する */
    bool start();

  private:
    EntryTask();

    static void entry(void *argument);
    void run();

    static EntryTask s_instance;
    Drivers m_drivers;
    Config m_config;
    BleAdvertiseTask &m_advertise_task;
    utkernel::task m_task;
};

}  // namespace task
}  // namespace app
