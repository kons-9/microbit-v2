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

namespace io {
class Stream;
}

namespace shell {

/* ================================================================== */
/*  Types                                                             */
/* ================================================================== */

/** コマンドハンドラ関数型 */
using CmdHandler = void (*)(int32_t argc, const char *const *argv);

/** コマンドエントリ */
struct Command {
    const char *name;   /**< コマンド名 */
    const char *help;   /**< ヘルプ文字列 (1行) */
    CmdHandler handler; /**< 実行関数 */
};

/* ================================================================== */
/*  API                                                               */
/* ================================================================== */

/**
 * @brief シェルを初期化する
 * @param stream      入出力ストリーム (UART 等)
 * @param extra_cmds  追加コマンドテーブル (NULLで組み込みのみ)
 * @param extra_count 追加コマンド数
 */
void init(io::Stream &stream, const Command *extra_cmds, uint8_t extra_count);

/**
 * @brief 1文字をシェルに入力する (UART RX割り込みから呼ぶ)
 * @param ch 受信文字
 *
 * 改行を検出するとコマンドをディスパッチする。
 */
void feed_char(char ch);

/**
 * @brief メインループから定期的に呼ぶ (ポーリング方式の場合)
 *
 * UART RX バッファからまとめて読み出し → FeedChar に渡す。
 */
void poll(void);

/**
 * @brief シェルに文字列を出力する (応答用)
 * @param str NULL終端文字列
 */
void puts(const char *str);

/**
 * @brief シェルにフォーマット出力する
 */
void printf(const char *fmt, ...);

}  // namespace shell
