#pragma once

/**
 * @file crash_info.h
 * @brief Crash Information — フォルト情報の構造体定義と操作API
 *
 * フォルトハンドラが Settings page に保存するクラッシュ情報を定義する。
 * main app / updater 両方から参照される。
 *
 * Settings page layout (0x0007F000, 4KB):
 *   +0x000: boot_mode (uint32_t) — SYSCONFIG_BOOT_APP or SYSCONFIG_BOOT_UPDATER
 *   +0x004: CrashInfo            — クラッシュ情報 (存在する場合)
 */

#include <cstdint>

#include <sysconfig.h>

/* ==================================================================
 * 定数
 * ================================================================== */

/** クラッシュ情報の有効性を示すマジックナンバー ("CRAS") */
constexpr uint32_t CRASH_INFO_MAGIC = 0x43524153U;

/** スタックダンプのワード数 */
constexpr uint32_t CRASH_STACK_DUMP_WORDS = 16;

/* ==================================================================
 * 型定義
 * ================================================================== */

/** フォルト種別 */
enum class CrashFaultType : uint32_t {
    Hard = 1,
    Mem = 2,
    Bus = 3,
    Usage = 4,
    NMI = 5,
};

/** クラッシュ情報構造体 */
struct CrashInfo {
    uint32_t magic;      /**< CRASH_INFO_MAGIC if valid */
    uint32_t fault_type; /**< CrashFaultType */

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
};

/**
 * フォルトハンドラの型
 *
 * @pre  frame は CPU が自動 push した exception frame (R0,R1,...,xPSR) を指す
 * @post この関数から戻ってはならない
 *
 * @param type        フォルト種別
 * @param frame       exception frame ポインタ
 * @param exc_return  EXC_RETURN 値 (LR at exception entry)
 */
using CrashHandlerCallback = void (*)(uint32_t type, uint32_t *frame, uint32_t exc_return);

/* ==================================================================
 * API
 * ================================================================== */

/**
 * フォルトハンドラを設定する
 *
 * 設定しない場合はデフォルトハンドラが使われる
 * (クラッシュ情報を Settings page に保存し updater モードで再起動)。
 *
 * @param callback  ハンドラ関数 (nullptr でデフォルトに戻す)
 */
void crash_set_handler(CrashHandlerCallback callback);

/**
 * Settings page からクラッシュ情報を読み出す
 *
 * @param info  読み出し先バッファ
 * @return 0 if valid crash info found, -1 if none
 */
int32_t crash_info_read(CrashInfo *info);

/**
 * Settings page のクラッシュ情報をクリアする (ブートモードは保持)
 */
void crash_info_clear(void);
