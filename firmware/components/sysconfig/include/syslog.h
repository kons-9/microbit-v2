#pragma once

/**
 * @file syslog.h
 * @brief Log Level Configuration — ログレベルの定義と切り替え
 *
 * C/C++ 両対応:
 *   - C++: enum class + constexpr
 *   - C  : fixed-width typedef + define
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

typedef uint8_t SyslogLevel;

#define SYSLOG_NONE ((SyslogLevel)0)
#define SYSLOG_ERROR ((SyslogLevel)1)
#define SYSLOG_WARN ((SyslogLevel)2)
#define SYSLOG_INFO ((SyslogLevel)3)
#define SYSLOG_DEBUG ((SyslogLevel)4)

/* TODO: CMake で SYSLOG_LEVEL を定義し、ビルド時に切り替える */
#ifndef SYSLOG_ACTIVE_LEVEL
#define SYSLOG_ACTIVE_LEVEL SYSLOG_DEBUG
#endif

#endif /* __cplusplus */
