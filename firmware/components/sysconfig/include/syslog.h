#pragma once

/**
 * @file syslog.h
 * @brief Log Level Configuration — ログレベルの定義と切り替え
 *
 * C/C++ 両対応:
 *   - C++: enum class + constexpr
 *   - C  : enum + define
 *
 * TODO: CMake で SYSLOG_LEVEL を定義し、ビルド時に切り替える
 */

#include <stdint.h>

#ifdef __cplusplus

namespace syslog {

enum class Level : uint8_t {
    None = 0,
    Error = 1,
    Warn = 2,
    Info = 3,
    Debug = 4,
};

/* TODO: CMake で SYSLOG_LEVEL を定義し、ビルド時に切り替える */
constexpr Level ACTIVE_LEVEL = Level::Debug;

} /* namespace syslog */

#else /* C */

typedef enum {
    SYSLOG_NONE = 0,
    SYSLOG_ERROR = 1,
    SYSLOG_WARN = 2,
    SYSLOG_INFO = 3,
    SYSLOG_DEBUG = 4,
} SyslogLevel;

/* TODO: CMake で SYSLOG_LEVEL を定義し、ビルド時に切り替える */
#ifndef SYSLOG_ACTIVE_LEVEL
#define SYSLOG_ACTIVE_LEVEL SYSLOG_DEBUG
#endif

#endif /* __cplusplus */
