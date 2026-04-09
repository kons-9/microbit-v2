/*
 * ble_service.cpp — BLE GATT service implementation
 */
#include "ble_service.h"
#include "arch/arch.h"

#include <cstdio>

namespace ble {

int BleService::init()
{
    int rc = arch::gatt_init();
    if (rc != 0) {
        std::fprintf(stderr, "GATT init failed: %d\n", rc);
        return rc;
    }
    initialized_ = true;
    std::printf("BLE GATT service initialized\n");
    return 0;
}

int BleService::notify_position(float x, float y, float confidence)
{
    if (!initialized_) return -1;

    int rc = arch::gatt_notify_position(x, y, confidence);
    if (rc != 0) {
        error_count_++;
        return rc;
    }
    notify_count_++;
    return 0;
}

int BleService::notify_mode(uint8_t mode)
{
    if (!initialized_) return -1;
    return arch::gatt_notify_mode(mode);
}

bool BleService::is_connected() const
{
    if (!initialized_) return false;
    return arch::gatt_is_connected();
}

void BleService::on_mode_request(ModeRequestCallback cb)
{
    arch::gatt_set_mode_callback(cb);
}

} // namespace ble
