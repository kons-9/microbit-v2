#pragma once

/**
 * @file shell.h
 * @brief 軽量 UART シェル
 *
 * コマンドテーブル駆動のシンプルなシェル。
 * UART からの1行入力をパースし、登録コマンドを実行する。
 *
 * 組み込みコマンド: ls, cat, erase, help
 * 外部コマンドはテーブル登録で追加可能。
 */

#include <cstdint>
#include <cstddef>

/* ================================================================== */
/*  Types                                                             */
/* ================================================================== */

/** コマンドハンドラ関数型 */
using ShellCmdHandler = void (*)(int32_t argc, const char *const *argv);

/** コマンドエントリ */
struct ShellCommand {
    const char *name;        /**< コマンド名 */
    const char *help;        /**< ヘルプ文字列 (1行) */
    ShellCmdHandler handler; /**< 実行関数 */
};

/* ================================================================== */
/*  API                                                               */
/* ================================================================== */

/**
 * @brief シェルを初期化する
 * @param extra_cmds  追加コマンドテーブル (NULLで組み込みのみ)
 * @param extra_count 追加コマンド数
 *
 * @pre UART が初期化済みであること
 */
void shell_init(const ShellCommand *extra_cmds, uint8_t extra_count);

/**
 * @brief 1文字をシェルに入力する (UART RX割り込みから呼ぶ)
 * @param ch 受信文字
 *
 * 改行を検出するとコマンドをディスパッチする。
 */
void shell_feed_char(char ch);

/**
 * @brief メインループから定期的に呼ぶ (ポーリング方式の場合)
 *
 * UART RX バッファからまとめて読み出し → FeedChar に渡す。
 */
void shell_poll(void);

/**
 * @brief シェルに文字列を出力する (応答用)
 * @param str NULL終端文字列
 */
void shell_puts(const char *str);

/**
 * @brief シェルにフォーマット出力する
 */
void shell_printf(const char *fmt, ...);
