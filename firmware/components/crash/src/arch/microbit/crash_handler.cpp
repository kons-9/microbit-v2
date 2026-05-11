/**
 * @file crash_handler.cpp
 * @brief Cortex-M フォルトハンドラ (micro:bit v2 / nRF52833)
 *
 * naked 関数で MSP/PSP を判別し、登録済みハンドラ (関数ポインタ) を呼ぶ。
 * デフォルトハンドラはクラッシュ情報を Settings page に保存し updater で再起動。
 *
 * TODO: Settings page への直接書き込みを ota arch API に委譲し、
 *       crash が永続化の詳細を知らない構成にする。
 */

#include "crash_info.h"

#include <nrfx_nvmc.h>
#include <nrf.h>

#include <cstring>

/* ==================================================================
 * デフォルトハンドラ: Flash 保存 → updater リブート
 * ================================================================== */

static void crash_default_handler(uint32_t type, uint32_t *frame, uint32_t exc_return) {
    CrashInfo info{};
    info.magic = CRASH_INFO_MAGIC;
    info.fault_type = type;

    /* Exception frame (CPU auto-pushed) */
    info.r0 = frame[0];
    info.r1 = frame[1];
    info.r2 = frame[2];
    info.r3 = frame[3];
    info.r12 = frame[4];
    info.lr = frame[5];
    info.pc = frame[6];
    info.xpsr = frame[7];

    /* Fault status registers */
    info.hfsr = SCB->HFSR;
    info.cfsr = SCB->CFSR;
    info.mmfar = SCB->MMFAR;
    info.bfar = SCB->BFAR;

    /* Context */
    info.sp = reinterpret_cast<uint32_t>(frame);
    info.exc_return = exc_return;

    /* Stack dump */
    uint32_t *above = frame + 8;
    for (int32_t i = 0; i < static_cast<int32_t>(CRASH_STACK_DUMP_WORDS); i++) {
        info.stack_dump[i] = above[i];
    }

    /* Settings page に保存 */
    uint32_t address = sysconfig_get_settings_address();
    nrfx_nvmc_page_erase(address);
    nrfx_nvmc_word_write(address, static_cast<uint32_t>(SYSCONFIG_BOOT_UPDATER));

    auto *words = reinterpret_cast<uint32_t *>(&info);
    uint32_t word_count = sizeof(CrashInfo) / sizeof(uint32_t);
    for (uint32_t i = 0; i < word_count; i++) {
        nrfx_nvmc_word_write(address + 4 + (i * 4), words[i]);
    }

    NVIC_SystemReset();
    while (1) {
        /* UNREACHABLE: NVIC_SystemReset() から戻ることはない */
    }
}

/* ==================================================================
 * 関数ポインタ
 * ================================================================== */

static CrashHandlerCallback s_handler = crash_default_handler;

void crash_set_handler(CrashHandlerCallback callback) {
    s_handler = (callback != nullptr) ? callback : crash_default_handler;
}

/* ==================================================================
 * 共通ディスパッチャ
 * ================================================================== */

static void crash_dispatch(uint32_t type, uint32_t *frame, uint32_t exc_return) {
    s_handler(type, frame, exc_return);
    while (1) {
        /* UNREACHABLE: handler must not return */
    }
}

/* ==================================================================
 * naked トランポリン — MSP/PSP を判別して crash_dispatch へ
 *
 * NOTE: naked 関数内では通常コードを書けないためインライン ASM のみ使用。
 * r0 = fault_type は各ハンドラで設定済み。
 * ================================================================== */

#define CRASH_TRAMPOLINE(name, fault_value)                                                                            \
    extern "C" __attribute__((naked)) void name(void) {                                                                \
        __asm volatile(                                                                                                \
            "mov  r0, %[ft]       \n" /* r0 = fault type */                                                            \
            "mov  r2, lr          \n" /* r2 = EXC_RETURN */                                                            \
            "tst  lr, #4          \n"                                                                                  \
            "ite  eq              \n"                                                                                  \
            "mrseq r1, msp        \n" /* MSP */                                                                        \
            "mrsne r1, psp        \n" /* PSP */                                                                        \
            "b    %[dispatch]     \n"                                                                                  \
            :                                                                                                          \
            : [ft] "i"(fault_value), [dispatch] "i"(crash_dispatch));                                                  \
    }

CRASH_TRAMPOLINE(HardFault_Handler, static_cast<uint32_t>(CrashFaultType::Hard))
CRASH_TRAMPOLINE(MemManage_Handler, static_cast<uint32_t>(CrashFaultType::Mem))
CRASH_TRAMPOLINE(BusFault_Handler, static_cast<uint32_t>(CrashFaultType::Bus))
CRASH_TRAMPOLINE(UsageFault_Handler, static_cast<uint32_t>(CrashFaultType::Usage))
CRASH_TRAMPOLINE(NMI_Handler, static_cast<uint32_t>(CrashFaultType::NMI))

/* ==================================================================
 * ユーティリティ
 * ================================================================== */

int32_t crash_info_read(CrashInfo *info) {
    const auto *source = reinterpret_cast<const uint32_t *>(
        sysconfig_get_settings_address() + 4);

    if (source[0] != CRASH_INFO_MAGIC) {
        return -1;
    }

    std::memcpy(info, source, sizeof(CrashInfo));
    return 0;
}

void crash_info_clear(void) {
    uint32_t address = sysconfig_get_settings_address();
    nrfx_nvmc_page_erase(address);
    nrfx_nvmc_word_write(address, static_cast<uint32_t>(SYSCONFIG_BOOT_APP));
}
