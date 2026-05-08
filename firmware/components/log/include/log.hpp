#pragma once

/**
 * @file log.hpp
 * @brief ログモジュール C++ ラッパー（header-only）
 *
 * C API をそのまま呼ぶ薄いラッパー。
 * テンプレートによる型安全性と、LOG_TAG の自動設定を提供する。
 *
 * 使い方:
 * @code
 *   #define LOG_TAG "BLE"
 *   #include "log.hpp"
 *
 *   log::info("scan started, interval=%d ms", interval);
 *   log::error("init failed: %d", err);
 *   log::hex_dump(log::Level::DEBUG, data, len);
 * @endcode
 */

#include "log.h"

namespace log {

enum class Level : int {
    Error = LOG_LEVEL_ERROR,
    Warn = LOG_LEVEL_WARN,
    Info = LOG_LEVEL_INFO,
    Debug = LOG_LEVEL_DEBUG,
};

inline void init(Level max_level = Level::Info) {
    LogInit(static_cast<LogLevel>(max_level));
}

inline void set_level(Level max_level) {
    LogSetLevel(static_cast<LogLevel>(max_level));
}

/* --- タグ付きログ出力 --- */

template <typename... Args>
inline void output(Level level, const char *tag, const char *fmt, Args... args) {
    LogOutput(static_cast<LogLevel>(level), tag, fmt, args...);
}

template <typename... Args>
inline void error(const char *fmt, Args... args) {
    LogOutput(LOG_LEVEL_ERROR, LOG_TAG, fmt, args...);
}

template <typename... Args>
inline void warn(const char *fmt, Args... args) {
    LogOutput(LOG_LEVEL_WARN, LOG_TAG, fmt, args...);
}

template <typename... Args>
inline void info(const char *fmt, Args... args) {
    LogOutput(LOG_LEVEL_INFO, LOG_TAG, fmt, args...);
}

template <typename... Args>
inline void debug(const char *fmt, Args... args) {
    LogOutput(LOG_LEVEL_DEBUG, LOG_TAG, fmt, args...);
}

/* --- Hex dump --- */

inline void hex_dump(Level level, const void *data, size_t len) {
    LogHexDump(static_cast<LogLevel>(level), LOG_TAG, data, len);
}

}  // namespace log
