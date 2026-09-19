#include <sysconfig.h>

#include <nrf.h>
#include <nrfx_nvmc.h>

namespace sysconfig {

[[noreturn]] void reboot(BootMode mode) {
    const uint32_t settings_address = get_settings_address();
    nrfx_nvmc_page_erase(settings_address);
    nrfx_nvmc_word_write(settings_address, static_cast<uint32_t>(mode));
    NVIC_SystemReset();

    for (;;) {
    }
}

}  // namespace sysconfig
