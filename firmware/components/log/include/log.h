#pragma once

/**
 * @file log.h
 * @brief 軽量ログモジュール
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
 *   %ld - long (符号付き10進)
 *   %lu - unsigned long (符号なし10進)
 *   %lx - unsigned long (16進, 小文字)
 *   %lX - unsigned long (16進, 大文字)
 *   %s  - const char* (文字列)
 *   %c  - char (1文字)
 *   %p  - void* (ポインタ, 0x付き16進)
 *   %%  - リテラル '%'
 *
 * 幅指定: %04x, %8d 等のゼロ埋め・右寄せに対応。
 */

#include <cstdint>
#include <cstddef>
#include <cstdarg>

#include "io_stream.h"

namespace logging {

/* ------------------------------------------------------------------ */
/* ログレベル定義                                                      */
/* ------------------------------------------------------------------ */

enum class LogLevel : uint8_t {
    Error = 0, /**< 致命的エラー */
    Warn = 1,  /**< 警告 */
    Info = 2,  /**< 情報 */
    Debug = 3, /**< デバッグ */
};

/**
 * @brief アプリケーション全体で共有するログ出力器
 *
 * UARTやFlash上のファイルなど、複数の出力先へ同じログを配信する。
 * インスタンスは Logger::instance() から取得する。
 */
class Logger {
  public:
    static constexpr size_t MaxStreams = 3;

    static Logger &instance();

    Logger(const Logger &) = delete;
    Logger &operator=(const Logger &) = delete;

    /** @brief 最大ログレベルと最初の出力先を設定する */
    void init(LogLevel max_level, ::io::Stream &stream);

    /** @brief 出力先を追加する。所有権は保持しない */
    bool add_stream(::io::Stream &stream);

    /** @brief 登録済みの出力先をすべて解除する */
    void clear_streams();

    /** @brief 実行時に最大ログレベルを変更する */
    void set_level(LogLevel max_level);

    /** @brief フォーマット付きログを出力する */
    void output(LogLevel level, const char *tag, const char *fmt, ...) __attribute__((format(printf, 4, 5)));

    /** @brief 生バイト列を16進ダンプする */
    void hex_dump(LogLevel level, const char *tag, const void *data, size_t len);

  private:
    static constexpr size_t OutputBufferSize = 512;

    Logger() = default;

    void put_char(char c);
    void put_str(const char *s);
    void put_uint(uint32_t val, int base, bool upper, int width, bool zero_pad);
    void put_int(int32_t val, int width, bool zero_pad);
    void log_vformat(const char *fmt, va_list ap);
    void flush_output();

    LogLevel max_level_ = LogLevel::Info;
    ::io::Stream *streams_[MaxStreams] = {};
    size_t stream_count_ = 0;
    uint8_t output_buffer_[OutputBufferSize] = {};
    size_t output_size_ = 0;
};

/* ------------------------------------------------------------------ */
/* 便利マクロ                                                          */
/* ------------------------------------------------------------------ */

#ifndef LOG_TAG
#define LOG_TAG "APP"
#endif

#define LOG_E(fmt, ...) ::logging::Logger::instance().output(::logging::LogLevel::Error, LOG_TAG, fmt, ##__VA_ARGS__)
#define LOG_W(fmt, ...) ::logging::Logger::instance().output(::logging::LogLevel::Warn, LOG_TAG, fmt, ##__VA_ARGS__)
#define LOG_I(fmt, ...) ::logging::Logger::instance().output(::logging::LogLevel::Info, LOG_TAG, fmt, ##__VA_ARGS__)
#define LOG_D(fmt, ...) ::logging::Logger::instance().output(::logging::LogLevel::Debug, LOG_TAG, fmt, ##__VA_ARGS__)

}  // namespace logging
