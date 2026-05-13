/**
 * @file log.cpp
 * @brief ログモジュール実装（printf非依存の軽量フォーマッタ）
 */

#include "log.h"
#include "io_stream.h"

#include <cstdarg>

/* ------------------------------------------------------------------ */
/* 内部状態                                                            */
/* ------------------------------------------------------------------ */

static LogLevel s_max_level = LOG_LEVEL_INFO;
static io::Stream *s_stream = nullptr;

/* ------------------------------------------------------------------ */
/* レベル文字列テーブル                                                */
/* ------------------------------------------------------------------ */

static const char *const s_level_tags[] = {
    "E", /* ERROR */
    "W", /* WARN  */
    "I", /* INFO  */
    "D", /* DEBUG */
};

/* ------------------------------------------------------------------ */
/* 低レベル出力ユーティリティ                                          */
/* ------------------------------------------------------------------ */

static inline void put_char(char c) {
    if (s_stream != nullptr) {
        s_stream->write(reinterpret_cast<const uint8_t *>(&c), 1);
    }
}

static inline void put_str(const char *s) {
    if (s == NULL) {
        if (s_stream != nullptr) {
            s_stream->write(reinterpret_cast<const uint8_t *>("(null)"), 6);
        }
        return;
    }
    while (*s) {
        put_char(*s++);
    }
}

/**
 * @brief 符号なし整数を指定基数で出力
 * @param val 出力する値
 * @param base 基数 (10 or 16)
 * @param upper 16進で大文字を使うか
 * @param width 最小幅 (0なら制限なし)
 * @param zero_pad ゼロ埋めするか
 */
static void put_uint(uint32_t val, int base, bool upper, int width, bool zero_pad) {
    char buf[12]; /* 32bit = 最大10桁(10進) or 8桁(16進) + 余裕 */
    int pos = 0;
    const char *digits = upper ? "0123456789ABCDEF" : "0123456789abcdef";

    if (val == 0) {
        buf[pos++] = '0';
    } else {
        while (val > 0) {
            buf[pos++] = digits[val % (unsigned)base];
            val /= (unsigned)base;
        }
    }

    /* パディング */
    char pad = zero_pad ? '0' : ' ';
    while (pos < width) {
        put_char(pad);
        width--;
    }

    /* 逆順で出力 */
    while (pos > 0) {
        put_char(buf[--pos]);
    }
}

/**
 * @brief 符号付き整数を10進で出力
 */
static void put_int(int32_t val, int width, bool zero_pad) {
    if (val < 0) {
        put_char('-');
        if (width > 0)
            width--;
        /* INT32_MINのオーバーフロー対策 */
        put_uint((uint32_t)(-(val + 1)) + 1, 10, false, width, zero_pad);
    } else {
        put_uint((uint32_t)val, 10, false, width, zero_pad);
    }
}

/* ------------------------------------------------------------------ */
/* フォーマッタ                                                        */
/* ------------------------------------------------------------------ */

static void log_vformat(const char *fmt, va_list ap) {
    while (*fmt) {
        if (*fmt != '%') {
            put_char(*fmt++);
            continue;
        }
        fmt++; /* skip '%' */

        /* フラグ解析 */
        bool zero_pad = false;
        if (*fmt == '0') {
            zero_pad = true;
            fmt++;
        }

        /* 幅解析 */
        int width = 0;
        while (*fmt >= '0' && *fmt <= '9') {
            width = width * 10 + (*fmt - '0');
            fmt++;
        }

        /* 長さ修飾子 */
        bool is_long = false;
        if (*fmt == 'l') {
            is_long = true;
            fmt++;
        }

        /* 指定子 */
        switch (*fmt) {
        case 'd': {
            int32_t v = is_long ? (int32_t)va_arg(ap, long) : va_arg(ap, int32_t);
            put_int(v, width, zero_pad);
            break;
        }
        case 'u': {
            uint32_t v = is_long ? (uint32_t)va_arg(ap, unsigned long) : va_arg(ap, uint32_t);
            put_uint(v, 10, false, width, zero_pad);
            break;
        }
        case 'x': {
            uint32_t v = is_long ? (uint32_t)va_arg(ap, unsigned long) : va_arg(ap, uint32_t);
            put_uint(v, 16, false, width, zero_pad);
            break;
        }
        case 'X': {
            uint32_t v = is_long ? (uint32_t)va_arg(ap, unsigned long) : va_arg(ap, uint32_t);
            put_uint(v, 16, true, width, zero_pad);
            break;
        }
        case 'p': {
            void *p = va_arg(ap, void *);
            put_str("0x");
            put_uint((uint32_t)(uintptr_t)p, 16, false, 8, true);
            break;
        }
        case 's': {
            const char *s = va_arg(ap, const char *);
            /* 幅指定は無視（文字列はそのまま出力） */
            put_str(s);
            break;
        }
        case 'c': {
            char c = (char)va_arg(ap, int);
            put_char(c);
            break;
        }
        case '%': put_char('%'); break;
        case '\0': return; /* 不正なフォーマット末尾 */
        default:
            /* 未対応指定子はそのまま出力 */
            put_char('%');
            put_char(*fmt);
            break;
        }
        fmt++;
    }
}

/* ------------------------------------------------------------------ */
/* 公開 API                                                            */
/* ------------------------------------------------------------------ */

void LogInit(LogLevel max_level, io::Stream &stream) {
    s_max_level = max_level;
    s_stream = &stream;
}

void LogSetLevel(LogLevel max_level) {
    s_max_level = max_level;
}

void LogOutput(LogLevel level, const char *tag, const char *fmt, ...) {
    if (level > s_max_level) {
        return;
    }

    /* プレフィックス: [E/TAG] */
    put_char('[');
    put_str(s_level_tags[level]);
    put_char('/');
    put_str(tag);
    put_char(']');
    put_char(' ');

    va_list ap;
    va_start(ap, fmt);
    log_vformat(fmt, ap);
    va_end(ap);

    put_char('\n');
}

void LogHexDump(LogLevel level, const char *tag, const void *data, size_t len) {
    if (level > s_max_level) {
        return;
    }

    const uint8_t *p = (const uint8_t *)data;
    const char *hex = "0123456789abcdef";

    /* ヘッダ */
    put_char('[');
    put_str(s_level_tags[level]);
    put_char('/');
    put_str(tag);
    put_char(']');
    put_str(" hex dump (");
    put_uint((uint32_t)len, 10, false, 0, false);
    put_str(" bytes):\n");

    for (size_t i = 0; i < len; i += 16) {
        /* アドレス */
        put_uint((uint32_t)i, 16, false, 4, true);
        put_str(": ");

        /* 16進 */
        for (size_t j = 0; j < 16; j++) {
            if (i + j < len) {
                put_char(hex[p[i + j] >> 4]);
                put_char(hex[p[i + j] & 0x0F]);
            } else {
                put_char(' ');
                put_char(' ');
            }
            put_char(' ');
            if (j == 7)
                put_char(' ');
        }

        put_str(" |");

        /* ASCII */
        for (size_t j = 0; j < 16 && (i + j) < len; j++) {
            char c = (char)p[i + j];
            put_char((c >= 0x20 && c <= 0x7E) ? c : '.');
        }

        put_str("|\n");
    }
}
