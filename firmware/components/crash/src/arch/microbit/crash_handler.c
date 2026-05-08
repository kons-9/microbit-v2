/**
 * crash_handler.c (micro:bit v2 / nRF52833)
 *
 * Cortex-M フォルトハンドラ。
 * naked 関数で MSP/PSP を判別し、登録済みハンドラ (関数ポインタ) を呼ぶ。
 * デフォルトハンドラはクラッシュ情報を Settings page に保存し updater で再起動。
 */

#include "crash_info.h"

#include <nrfx_nvmc.h>
#include <nrf.h>
#include <string.h>

/* ------------------------------------------------------------------ */
/*  デフォルトハンドラ: Flash 保存 → updater リブート                  */
/*  TODO: Settings page への直接書き込みを ota arch API に委譲し、     */
/*        crash が永続化の詳細を知らない構成にできる。                  */
/*        (e.g. ota_arch_persist_crash(data, size) + ota_arch_reboot)  */
/* ------------------------------------------------------------------ */

static void crash_default_handler(crash_fault_type_t type, uint32_t *frame, uint32_t exc_return) {
    crash_info_t info;
    info.magic = CRASH_INFO_MAGIC;
    info.fault_type = (uint32_t)type;

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
    info.sp = (uint32_t)frame;
    info.exc_return = exc_return;

    /* Stack dump */
    uint32_t *above = frame + 8;
    for (int i = 0; i < CRASH_STACK_DUMP_WORDS; i++) {
        info.stack_dump[i] = above[i];
    }

    /* Settings page に保存 */
    uint32_t addr = sysconfig_settings_addr();
    nrfx_nvmc_page_erase(addr);
    nrfx_nvmc_word_write(addr, (uint32_t)SYSCONFIG_BOOT_UPDATER);

    uint32_t *words = (uint32_t *)&info;
    uint32_t n_words = sizeof(crash_info_t) / sizeof(uint32_t);
    for (uint32_t i = 0; i < n_words; i++) {
        nrfx_nvmc_word_write(addr + 4 + (i * 4), words[i]);
    }

    NVIC_SystemReset();
    while (1)
        ;
}

/* ------------------------------------------------------------------ */
/*  関数ポインタ                                                       */
/* ------------------------------------------------------------------ */

static crash_handler_fn s_handler = crash_default_handler;

void crash_set_handler(crash_handler_fn fn) {
    s_handler = fn ? fn : crash_default_handler;
}

/* ------------------------------------------------------------------ */
/*  共通ディスパッチャ (通常の C 関数)                                 */
/* ------------------------------------------------------------------ */

static void crash_dispatch(crash_fault_type_t type, uint32_t *frame, uint32_t exc_return) {
    s_handler(type, frame, exc_return);
    while (1)
        ; /* handler must not return */
}

/* ------------------------------------------------------------------ */
/*  naked トランポリン — MSP/PSP を判別して crash_dispatch へ          */
/*                                                                     */
/*  naked 関数内では C コードを書けないためインライン ASM のみ使用。    */
/*  r0 = fault_type は各ハンドラで設定済み。                           */
/* ------------------------------------------------------------------ */

#define CRASH_TRAMPOLINE(name, fault_val)                                                                              \
    __attribute__((naked)) void name(void) {                                                                           \
        __asm volatile(                                                                                                \
            "mov  r0, %[ft]       \n" /* r0 = fault type */                                                            \
            "mov  r2, lr          \n" /* r2 = EXC_RETURN */                                                            \
            "tst  lr, #4          \n"                                                                                  \
            "ite  eq              \n"                                                                                  \
            "mrseq r1, msp        \n" /* MSP */                                                                        \
            "mrsne r1, psp        \n" /* PSP */                                                                        \
            "b    %[dispatch]     \n"                                                                                  \
            :                                                                                                          \
            : [ft] "i"(fault_val), [dispatch] "i"(crash_dispatch));                                                    \
    }

CRASH_TRAMPOLINE(HardFault_Handler, CRASH_FAULT_HARD)
CRASH_TRAMPOLINE(MemManage_Handler, CRASH_FAULT_MEM)
CRASH_TRAMPOLINE(BusFault_Handler, CRASH_FAULT_BUS)
CRASH_TRAMPOLINE(UsageFault_Handler, CRASH_FAULT_USAGE)
CRASH_TRAMPOLINE(NMI_Handler, CRASH_FAULT_NMI)

/* ------------------------------------------------------------------ */
/*  ユーティリティ                                                     */
/* ------------------------------------------------------------------ */

int crash_info_read(crash_info_t *info) {
    const uint32_t *src = (const uint32_t *)(sysconfig_settings_addr() + 4);

    if (src[0] != CRASH_INFO_MAGIC) {
        return -1;
    }

    memcpy(info, src, sizeof(crash_info_t));
    return 0;
}

void crash_info_clear(void) {
    uint32_t addr = sysconfig_settings_addr();
    nrfx_nvmc_page_erase(addr);
    nrfx_nvmc_word_write(addr, (uint32_t)SYSCONFIG_BOOT_APP);
}
