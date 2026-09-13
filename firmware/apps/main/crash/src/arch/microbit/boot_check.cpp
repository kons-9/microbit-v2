/**
 * @file boot_check.cpp
 * @brief Reset Handlerラッパー — ブートモード判定
 */

#include <sysconfig.h>

#include <cstdint>

extern "C" void __real_Reset_Handler(void);

extern "C" void __wrap_Reset_Handler(void) {
    volatile uint32_t *settings = sysconfig::get_settings_pointer();

    if (*settings == static_cast<uint32_t>(sysconfig::BOOT_UPDATER)) {
        volatile uint32_t *updater_vector_table = sysconfig::get_updater_vector_table();
        uint32_t stack_pointer = updater_vector_table[0];
        uint32_t program_counter = updater_vector_table[1];

        __asm volatile(
            "msr msp, %0 \n"
            "bx  %1      \n"
            :
            : "r"(stack_pointer), "r"(program_counter));
    }

    __real_Reset_Handler();
}
