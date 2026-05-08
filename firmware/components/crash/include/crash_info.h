#pragma once

/**
 * Crash Information Structure
 *
 * フォルトハンドラがSettings pageに保存するクラッシュ情報。
 * main app / updater 両方から参照される。
 *
 * Settings page layout (0x0007F000, 4KB):
 *   +0x000: boot_mode (uint32_t) — SYSCONFIG_BOOT_APP or SYSCONFIG_BOOT_UPDATER
 *   +0x004: crash_info_t          — クラッシュ情報 (存在する場合)
 */

#include <stdint.h>
#include <sysconfig.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ---- Crash Info ---- */

#define CRASH_INFO_MAGIC 0x43524153 /* "CRAS" */
#define CRASH_STACK_DUMP_WORDS 16

typedef enum {
    CRASH_FAULT_HARD = 1,
    CRASH_FAULT_MEM = 2,
    CRASH_FAULT_BUS = 3,
    CRASH_FAULT_USAGE = 4,
    CRASH_FAULT_NMI = 5,
} crash_fault_type_t;

typedef struct {
    uint32_t magic;      /**< CRASH_INFO_MAGIC if valid */
    uint32_t fault_type; /**< crash_fault_type_t */

    /* Exception frame (CPU が自動 push した値) */
    uint32_t r0;
    uint32_t r1;
    uint32_t r2;
    uint32_t r3;
    uint32_t r12;
    uint32_t lr; /**< リンクレジスタ (呼び出し元) */
    uint32_t pc; /**< フォルト発生時のプログラムカウンタ */
    uint32_t xpsr;

    /* Fault status registers */
    uint32_t hfsr;  /**< Hard Fault Status Register */
    uint32_t cfsr;  /**< Configurable Fault Status Register */
    uint32_t mmfar; /**< MemManage Fault Address Register */
    uint32_t bfar;  /**< Bus Fault Address Register */

    /* Context */
    uint32_t sp;         /**< フォルト時のスタックポインタ */
    uint32_t exc_return; /**< EXC_RETURN (LR at exception entry) */

    /* Stack dump (フォルト時のスタック上位Nワード) */
    uint32_t stack_dump[CRASH_STACK_DUMP_WORDS];
} crash_info_t;

/**
 * フォルトハンドラの型
 *
 * @param type       フォルト種別
 * @param frame      CPU が自動 push した exception frame (R0,R1,...,xPSR)
 * @param exc_return EXC_RETURN 値 (LR at exception entry)
 *
 * この関数から戻ってはならない。
 */
typedef void (*crash_handler_fn)(crash_fault_type_t type, uint32_t *frame, uint32_t exc_return);

/**
 * フォルトハンドラを設定する
 *
 * 設定しない場合はデフォルトハンドラが使われる
 * (クラッシュ情報を Settings page に保存し updater モードで再起動)。
 *
 * @param fn  ハンドラ関数 (NULL でデフォルトに戻す)
 */
void crash_set_handler(crash_handler_fn fn);

/**
 * Settings page からクラッシュ情報を読み出す
 * @param info  読み出し先
 * @return 0 if valid crash info found, -1 if none
 */
int crash_info_read(crash_info_t *info);

/**
 * Settings page のクラッシュ情報をクリア (ブートモードは保持)
 */
void crash_info_clear(void);

#ifdef __cplusplus
}
#endif
