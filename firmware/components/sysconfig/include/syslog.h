#pragma once

/**
 * syslog.h — Log Level Configuration
 *
 * C/C++ 両対応:
 *   - C++: enum class + constexpr
 *   - C  : enum + define
 */

#include <stdint.h>

#ifdef __cplusplus

namespace syslog {

enum class level : uint8_t {
    none = 0,
    error = 1,
    warn = 2,
    info = 3,
    debug = 4,
};

// TODO: CMake で SYSLOG_LEVEL を定義し、ビルド時に切り替える
constexpr level active_level = level::debug;

} /* namespace syslog */

#else /* C */

typedef enum {
    SYSLOG_NONE = 0,
    SYSLOG_ERROR = 1,
    SYSLOG_WARN = 2,
    SYSLOG_INFO = 3,
    SYSLOG_DEBUG = 4,
} syslog_level_t;

// TODO: CMake で SYSLOG_LEVEL を定義し、ビルド時に切り替える
#ifndef SYSLOG_ACTIVE_LEVEL
#define SYSLOG_ACTIVE_LEVEL SYSLOG_DEBUG
#endif

#endif /* __cplusplus */
