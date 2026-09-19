/**
 * @file crash_handler.cpp
 * @brief Cortex-Mフォルトハンドラ (micro:bit v2 / nRF52833)
 */

#include "crash_info.h"

#include <nrfx_nvmc.h>
#include <nrf.h>
#include <sysconfig.h>

#include <cstring>

namespace crash {

CrashHandler CrashHandler::s_instance;

CrashHandler &CrashHandler::instance() {
    return s_instance;
}

void CrashHandler::set_handler(HandlerCallback callback) {
    m_handler = callback;
}

[[noreturn]] void CrashHandler::default_handler(uint32_t type, uint32_t *frame, uint32_t exc_return) {
    Info info{};
    info.magic = INFO_MAGIC;
    info.fault_type = type;

    info.r0 = frame[0];
    info.r1 = frame[1];
    info.r2 = frame[2];
    info.r3 = frame[3];
    info.r12 = frame[4];
    info.lr = frame[5];
    info.pc = frame[6];
    info.xpsr = frame[7];

    info.hfsr = SCB->HFSR;
    info.cfsr = SCB->CFSR;
    info.mmfar = SCB->MMFAR;
    info.bfar = SCB->BFAR;

    info.sp = reinterpret_cast<uint32_t>(frame);
    info.exc_return = exc_return;

    uint32_t *above = frame + 8;
    for (uint32_t i = 0; i < STACK_DUMP_WORDS; i++) {
        info.stack_dump[i] = above[i];
    }

    uint32_t address = sysconfig::get_settings_address();
    nrfx_nvmc_page_erase(address);
    nrfx_nvmc_word_write(address, static_cast<uint32_t>(sysconfig::BOOT_RECOVERY));

    auto *words = reinterpret_cast<uint32_t *>(&info);
    uint32_t word_count = sizeof(Info) / sizeof(uint32_t);
    for (uint32_t i = 0; i < word_count; i++) {
        nrfx_nvmc_word_write(address + 4 + (i * 4), words[i]);
    }

    NVIC_SystemReset();
    while (true) {
    }
}

[[noreturn]] void CrashHandler::dispatch(uint32_t type, uint32_t *frame, uint32_t exc_return) {
    auto handler = m_handler;
    if (handler == nullptr) {
        handler = default_handler;
    }
    handler(type, frame, exc_return);
    while (true) {
    }
}

int32_t CrashHandler::info_read(Info *info) const {
    if (info == nullptr) {
        return -1;
    }

    const auto *source = reinterpret_cast<const uint32_t *>(sysconfig::get_settings_address() + 4);
    if (source[0] != INFO_MAGIC) {
        return -1;
    }

    std::memcpy(info, source, sizeof(Info));
    return 0;
}

void CrashHandler::info_clear() const {
    uint32_t address = sysconfig::get_settings_address();
    nrfx_nvmc_page_erase(address);
    nrfx_nvmc_word_write(address, static_cast<uint32_t>(sysconfig::BOOT_APP));
}

}  // namespace crash

extern "C" [[noreturn]] void crash_dispatch(uint32_t type, uint32_t *frame, uint32_t exc_return) {
    crash::CrashHandler::instance().dispatch(type, frame, exc_return);
}

#define CRASH_TRAMPOLINE(name, fault_value)                                                                            \
    extern "C" __attribute__((naked)) void name(void) {                                                                \
        __asm volatile(                                                                                                \
            "mov  r0, %[ft]       \n"                                                                                  \
            "mov  r2, lr          \n"                                                                                  \
            "tst  lr, #4          \n"                                                                                  \
            "ite  eq              \n"                                                                                  \
            "mrseq r1, msp        \n"                                                                                  \
            "mrsne r1, psp        \n"                                                                                  \
            "b    %[dispatch]     \n"                                                                                  \
            :                                                                                                          \
            : [ft] "i"(fault_value), [dispatch] "i"(crash_dispatch));                                                  \
    }

CRASH_TRAMPOLINE(HardFault_Handler, static_cast<uint32_t>(crash::FaultType::Hard))
CRASH_TRAMPOLINE(MemManage_Handler, static_cast<uint32_t>(crash::FaultType::Mem))
CRASH_TRAMPOLINE(BusFault_Handler, static_cast<uint32_t>(crash::FaultType::Bus))
CRASH_TRAMPOLINE(UsageFault_Handler, static_cast<uint32_t>(crash::FaultType::Usage))
CRASH_TRAMPOLINE(NMI_Handler, static_cast<uint32_t>(crash::FaultType::NMI))
