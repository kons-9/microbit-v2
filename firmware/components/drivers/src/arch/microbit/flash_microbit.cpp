/** @file flash_microbit.cpp */

#include "flash.h"

#include <nrfx_nvmc.h>
#include <cstring>

namespace drivers {

void Flash::page_erase(uint32_t address) {
    nrfx_nvmc_page_erase(address);
}

void Flash::write(uint32_t address, const void *data, size_t size) {
    nrfx_nvmc_bytes_write(address, data, size);
}

void Flash::read(uint32_t address, void *data, size_t size) {
    std::memcpy(data, reinterpret_cast<const void *>(address), size);
}

}  // namespace drivers
