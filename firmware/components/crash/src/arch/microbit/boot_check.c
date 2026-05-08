/**
 * boot_check.c
 *
 * Reset_Handler をラップし、Settings page のブートモードフラグを確認する。
 * SYSCONFIG_BOOT_UPDATER なら updater スロットのベクタテーブルへジャンプ。
 *
 * -Wl,--wrap=Reset_Handler でリンクされることで有効になる。
 * main app のみで使用 (updater は自分自身にジャンプしてはいけない)。
 */

#include <sysconfig.h>
#include <stdint.h>

/* カーネルの元の Reset_Handler (--wrap により __real_ プレフィックス付き) */
extern void __real_Reset_Handler(void);

/**
 * ラップされた Reset_Handler
 *
 * .data/.bss 初期化前に実行されるため、グローバル変数は使用不可。
 * Flash は直接メモリマップされているため読み出しは初期化不要。
 */
void __wrap_Reset_Handler(void) {
    volatile uint32_t *settings = sysconfig_settings_ptr();

    if (*settings == SYSCONFIG_BOOT_UPDATER) {
        volatile uint32_t *updater_vt = sysconfig_updater_vt();
        uint32_t sp = updater_vt[0];
        uint32_t pc = updater_vt[1];

        __asm volatile(
            "msr msp, %0 \n"
            "bx  %1      \n"
            :
            : "r"(sp), "r"(pc));
    }

    /* 通常起動 → カーネルの元の Reset_Handler へ */
    __real_Reset_Handler();
}
