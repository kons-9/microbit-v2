/**
 * @file main.cpp
 * @brief BLE Locator main application entry point
 */

#include "entry_task.h"

#include <utkernel/task>

extern "C" int app_main(void) {
    if (!app::task::EntryTask::instance().start()) {
        return -1;
    }

    utkernel::task::sleep_forever();
    return 0;
}
