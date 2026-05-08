/**
 * @file shell.cpp
 * @brief 軽量 UART シェル実装
 */

#include "shell.h"
#include "flash_fs.h"
#include "uart.h"

#include <cstring>
#include <cstdio>
#include <cstdarg>

/* ================================================================== */
/*  Constants                                                         */
/* ================================================================== */

static constexpr size_t LINE_BUF_SIZE = 128;
static constexpr size_t MAX_ARGS = 8;
static constexpr char PROMPT[] = "> ";

/* ================================================================== */
/*  State                                                             */
/* ================================================================== */

static char s_lineBuf[LINE_BUF_SIZE];
static size_t s_linePos = 0;

static const ShellCommand *s_extraCmds = nullptr;
static uint8_t s_extraCount = 0;

/* ================================================================== */
/*  Output helpers                                                    */
/* ================================================================== */

void shell_puts(const char *str) {
    uart_puts(str);
}

void shell_printf(const char *fmt, ...) {
    char buf[256];
    va_list ap;
    va_start(ap, fmt);
    int len = vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    if (len > 0) {
        uart_write(reinterpret_cast<const uint8_t *>(buf), static_cast<size_t>(len));
    }
}

/* ================================================================== */
/*  Built-in commands                                                 */
/* ================================================================== */

static void cmd_help(int32_t argc, const char *const *argv);
static void cmd_ls(int32_t argc, const char *const *argv);
static void cmd_cat(int32_t argc, const char *const *argv);
static void cmd_erase(int32_t argc, const char *const *argv);

static const ShellCommand BUILTIN_CMDS[] = {
    {"help", "Show available commands", cmd_help},
    {"ls", "List flash files", cmd_ls},
    {"cat", "cat <file> [--hex] - Read file", cmd_cat},
    {"erase", "erase <file> - Erase file", cmd_erase},
};

static constexpr uint8_t BUILTIN_COUNT = static_cast<uint8_t>(sizeof(BUILTIN_CMDS) / sizeof(BUILTIN_CMDS[0]));

static void cmd_help(int32_t /*argc*/, const char *const * /*argv*/) {
    shell_puts("Commands:\r\n");
    for (uint8_t i = 0; i < BUILTIN_COUNT; ++i) {
        shell_printf("  %-8s %s\r\n", BUILTIN_CMDS[i].name, BUILTIN_CMDS[i].help);
    }
    for (uint8_t i = 0; i < s_extraCount; ++i) {
        shell_printf("  %-8s %s\r\n", s_extraCmds[i].name, s_extraCmds[i].help);
    }
}

static void cmd_ls(int32_t /*argc*/, const char *const * /*argv*/) {
    shell_printf("%-10s %6s/%6s  %s\r\n", "NAME", "USED", "CAP", "MODE");
    for (uint8_t i = 0; i < FLASH_FS_FILE_COUNT; ++i) {
        FlashFsFileInfo info;
        if (flash_fs_get_info(static_cast<FlashFsFileId>(i), &info)) {
            const char *mode_str = (info.mode == FLASH_FS_MODE_STREAM) ? "stream" : "block";
            shell_printf("%-10s %6lu/%6lu  [%s]\r\n",
                         info.name,
                         static_cast<unsigned long>(info.used),
                         static_cast<unsigned long>(info.capacity),
                         mode_str);
        }
    }
}

static void cmd_cat(int32_t argc, const char *const *argv) {
    if (argc < 2) {
        shell_puts("Usage: cat <file> [--hex]\r\n");
        return;
    }

    FlashFsFileId id = flash_fs_find_by_name(argv[1]);
    if (id == FLASH_FS_FILE_COUNT) {
        shell_printf("Unknown file: %s\r\n", argv[1]);
        return;
    }

    bool hex_mode = false;
    if (argc >= 3 && std::strcmp(argv[2], "--hex") == 0) {
        hex_mode = true;
    }

    FlashFsFileInfo info;
    flash_fs_get_info(id, &info);

    // 256バイトずつ読み出し
    uint8_t buf[256];
    uint32_t offset = 0;
    uint32_t total = info.used;

    while (offset < total) {
        size_t chunk = sizeof(buf);
        if (offset + chunk > total) {
            chunk = total - offset;
        }

        size_t read_len = 0;
        if (info.mode == FLASH_FS_MODE_STREAM) {
            read_len = flash_fs_read(id, offset, buf, chunk);
        } else {
            read_len = flash_fs_block_read(id, offset, buf, chunk);
        }

        if (read_len == 0) {
            break;
        }

        if (hex_mode) {
            for (size_t i = 0; i < read_len; ++i) {
                if (i % 16 == 0) {
                    shell_printf("%08lX: ", static_cast<unsigned long>(offset + i));
                }
                shell_printf("%02X ", buf[i]);
                if (i % 16 == 15 || i == read_len - 1) {
                    shell_puts("\r\n");
                }
            }
        } else {
            uart_write(buf, read_len);
        }

        offset += static_cast<uint32_t>(read_len);
    }
    shell_puts("\r\n");
}

static void cmd_erase(int32_t argc, const char *const *argv) {
    if (argc < 2) {
        shell_puts("Usage: erase <file>\r\n");
        return;
    }

    FlashFsFileId id = flash_fs_find_by_name(argv[1]);
    if (id == FLASH_FS_FILE_COUNT) {
        shell_printf("Unknown file: %s\r\n", argv[1]);
        return;
    }

    flash_fs_erase(id);
    shell_puts("OK\r\n");
}

/* ================================================================== */
/*  Line parsing & dispatch                                           */
/* ================================================================== */

static void dispatch_line(void) {
    // 引数分割
    const char *args[MAX_ARGS] = {};
    int32_t argc = 0;

    char *p = s_lineBuf;
    while (*p != '\0' && argc < static_cast<int32_t>(MAX_ARGS)) {
        // スペースをスキップ
        while (*p == ' ') {
            ++p;
        }
        if (*p == '\0') {
            break;
        }
        args[argc++] = p;
        // トークン末尾まで進む
        while (*p != '\0' && *p != ' ') {
            ++p;
        }
        if (*p != '\0') {
            *p++ = '\0';
        }
    }

    if (argc == 0) {
        return;
    }

    // コマンド検索
    for (uint8_t i = 0; i < BUILTIN_COUNT; ++i) {
        if (std::strcmp(args[0], BUILTIN_CMDS[i].name) == 0) {
            BUILTIN_CMDS[i].handler(argc, args);
            return;
        }
    }
    for (uint8_t i = 0; i < s_extraCount; ++i) {
        if (std::strcmp(args[0], s_extraCmds[i].name) == 0) {
            s_extraCmds[i].handler(argc, args);
            return;
        }
    }

    shell_printf("Unknown command: %s\r\n", args[0]);
}

/* ================================================================== */
/*  Public API                                                        */
/* ================================================================== */

void shell_init(const ShellCommand *extra_cmds, uint8_t extra_count) {
    s_extraCmds = extra_cmds;
    s_extraCount = extra_count;
    s_linePos = 0;
    shell_puts(PROMPT);
}

void shell_feed_char(char ch) {
    if (ch == '\r' || ch == '\n') {
        shell_puts("\r\n");
        s_lineBuf[s_linePos] = '\0';
        dispatch_line();
        s_linePos = 0;
        shell_puts(PROMPT);
        return;
    }

    // Backspace
    if (ch == '\b' || ch == 0x7F) {
        if (s_linePos > 0) {
            --s_linePos;
            shell_puts("\b \b");
        }
        return;
    }

    // 通常文字
    if (s_linePos < LINE_BUF_SIZE - 1) {
        s_lineBuf[s_linePos++] = ch;
        // エコー
        uart_write(reinterpret_cast<const uint8_t *>(&ch), 1);
    }
}

void shell_poll(void) {
    uint8_t buf[32];
    int32_t len = uart_read(buf, sizeof(buf), 0);
    for (int32_t i = 0; i < len; ++i) {
        shell_feed_char(static_cast<char>(buf[i]));
    }
}
