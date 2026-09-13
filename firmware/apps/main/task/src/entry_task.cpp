/**
 * @file entry_task.cpp
 * @brief メインアプリケーションのエントリータスク実装
 */

#define LOG_TAG "MAIN"
#include "entry_task.h"

#include "log.h"
#include "sysconfig.h"

namespace app::task {

EntryTask EntryTask::s_instance;

EntryTask &EntryTask::instance() {
    return s_instance;
}

EntryTask::EntryTask()
    : m_drivers()
    , m_config(m_drivers)
    , m_advertise_task(BleAdvertiseTask::instance()) {
}

bool EntryTask::start() {
    if (m_task.joinable()) {
        return false;
    }

    utkernel::task::config task_config;
    task_config.name = "entry";
    task_config.priority = 8;
    task_config.stack_size = 4096;
    task_config.param = this;

    if (!m_task.create(entry, task_config)) {
        return false;
    }
    if (!m_task.start()) {
        m_task.terminate();
        return false;
    }
    return true;
}

void EntryTask::entry(void *argument) {
    static_cast<EntryTask *>(argument)->run();
}

void EntryTask::run() {
    m_config.drivers.uart.init();
    logging::Logger::instance().init(logging::LogLevel::Debug, m_config.drivers.uart);

    m_config.drivers.button.init();
    if (m_config.drivers.button.is_pressed(drivers::ButtonId::A)) {
        LOG_I("A pressed: entering log reader mode");
        sysconfig::reboot(sysconfig::BOOT_UPDATER);
    }

    auto fs_result = m_config.file_system.init();
    if (fs_result != 0) {
        LOG_E("fs init failed: %ld", static_cast<long>(fs_result));
        utkernel::task::sleep_forever();
        return;
    }

    logging::Logger::instance().add_stream(m_config.log_file);

    if (!m_advertise_task.start()) {
        LOG_E("BLE advertising task create/start failed");
    }

    utkernel::task::sleep_forever();
}

}  // namespace app::task
