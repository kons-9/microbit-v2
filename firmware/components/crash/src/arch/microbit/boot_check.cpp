/**
 * @file boot_check.cpp
 * @brief Reset Handler ラッパー — ブートモード判定
 *
 * Reset_Handler をラップし、Settings page のブートモードフラグを確認する。
 * SYSCONFIG_BOOT_UPDATER なら updater スロットのベクタテーブルへジャンプ。
 *
 * NOTE: -Wl,--wrap=Reset_Handler でリンクされることで有効になる。
 * NOTE: main app のみで使用 (updater は自分自身にジャンプしてはいけない)。
 * NOTE: .data/.bss 初期化前に実行されるため、グローバル変数は使用不可。
 */

#include <sysconfig.h>

#include <cstdint>

/* カーネルの元の Reset_Handler (--wrap により __real_ プレフィックス付き) */
extern "C" void __real_Reset_Handler(void);

/**
 * ラップされた Reset_Handler
 *
 * @pre  Flash は直接メモリマップされているため読み出しは初期化不要
 * @post ブートモードに応じて updater へジャンプ、または通常起動
 */
extern "C" void __wrap_Reset_Handler(void) {
    volatile uint32_t *settings = sysconfig_get_settings_pointer();

    if (*settings == static_cast<uint32_t>(SYSCONFIG_BOOT_UPDATER)) {
        volatile uint32_t *updater_vector_table = sysconfig_get_updater_vector_table();
        uint32_t stack_pointer = updater_vector_table[0];
        uint32_t program_counter = updater_vector_table[1];

        __asm volatile(
            "msr msp, %0 \n"
            "bx  %1      \n"
            :
            : "r"(stack_pointer), "r"(program_counter));
    }

    /* 通常起動 → カーネルの元の Reset_Handler へ */
    __real_Reset_Handler();
}
