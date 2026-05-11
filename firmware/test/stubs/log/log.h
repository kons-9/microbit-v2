#pragma once

// Stub log.h for Linux test builds — all macros expand to no-ops

#ifndef LOG_TAG
#define LOG_TAG "TEST"
#endif

#define LOG_E(fmt, ...) ((void)0)
#define LOG_W(fmt, ...) ((void)0)
#define LOG_I(fmt, ...) ((void)0)
#define LOG_D(fmt, ...) ((void)0)
