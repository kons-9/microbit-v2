#pragma once

/**
 * @file log.h
 * @brief 軽量ログモジュール（C API）
 *
 * UARTバックエンドに出力するログ機能を提供する。
 * ログレベルによるフィルタリングに対応。
 * フォーマット文字列は最小限の独自実装（printf非依存）。
 *
 * 対応フォーマット指定子:
 *   %d  - int32_t (符号付き10進)
 *   %u  - uint32_t (符号なし10進)
 *   %x  - uint32_t (16進, 小文字)
 *   %X  - uint32_t (16進, 大文字)
 *   %s  - const char* (文字列)
 *   %c  - char (1文字)
 *   %p  - void* (ポインタ, 0x付き16進)
 *   %%  - リテラル '%'
 *
 * 幅指定: %04x, %8d 等のゼロ埋め・右寄せに対応。
 */

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------------------------------------------ */
/* ログレベル定義                                                      */
/* ------------------------------------------------------------------ */

typedef enum {
    LOG_LEVEL_ERROR = 0, /**< 致命的エラー */
    LOG_LEVEL_WARN = 1,  /**< 警告 */
    LOG_LEVEL_INFO = 2,  /**< 情報 */
    LOG_LEVEL_DEBUG = 3, /**< デバッグ */
} LogLevel;

/* ------------------------------------------------------------------ */
/* 初期化・設定                                                        */
/* ------------------------------------------------------------------ */

/**
 * @brief ログモジュールを初期化する
 * @param max_level このレベル以下のログのみ出力する
 *
 * 内部で UARTInit を呼ぶ。UART未初期化の状態で呼んでよい。
 */
void LogInit(LogLevel max_level);

/**
 * @brief 実行時にログレベルを変更する
 * @param max_level 新しい最大ログレベル
 */
void LogSetLevel(LogLevel max_level);

/* ------------------------------------------------------------------ */
/* ログ出力 (直接呼び出し用)                                           */
/* ------------------------------------------------------------------ */

/**
 * @brief フォーマット付きログ出力
 * @param level ログレベル
 * @param tag モジュール名タグ (例: "BLE", "OTA")
 * @param fmt フォーマット文字列
 *
 * max_level より大きい level のメッセージは出力されない。
 */
void LogOutput(LogLevel level, const char *tag, const char *fmt, ...) __attribute__((format(printf, 3, 4)));

/**
 * @brief 生バイト列を16進ダンプする
 * @param level ログレベル
 * @param tag モジュール名タグ
 * @param data ダンプ対象データ
 * @param len データ長(バイト)
 */
void LogHexDump(LogLevel level, const char *tag, const void *data, size_t len);

/* ------------------------------------------------------------------ */
/* 便利マクロ                                                          */
/* ------------------------------------------------------------------ */

#ifndef LOG_TAG
#define LOG_TAG "APP"
#endif

#define LOG_E(fmt, ...) LogOutput(LOG_LEVEL_ERROR, LOG_TAG, fmt, ##__VA_ARGS__)
#define LOG_W(fmt, ...) LogOutput(LOG_LEVEL_WARN, LOG_TAG, fmt, ##__VA_ARGS__)
#define LOG_I(fmt, ...) LogOutput(LOG_LEVEL_INFO, LOG_TAG, fmt, ##__VA_ARGS__)
#define LOG_D(fmt, ...) LogOutput(LOG_LEVEL_DEBUG, LOG_TAG, fmt, ##__VA_ARGS__)

#ifdef __cplusplus
}
#endif
