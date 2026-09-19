/**
 * @file shell.cpp
 * @brief 軽量 UART シェル実装
 */

#include "shell.h"
#include "io_stream.h"

#include <cstdarg>
#include <cstdio>
#include <cstring>

namespace shell {

/* ================================================================== */
/*  Output helpers                                                    */
/* ================================================================== */

void Shell::puts(const char *str) {
    if (stream_ != nullptr && str != nullptr) {
        stream_->write(reinterpret_cast<const uint8_t *>(str), std::strlen(str));
    }
}

void Shell::vprintf(const char *fmt, va_list ap) {
    if (stream_ == nullptr || fmt == nullptr) {
        return;
    }

    int len = std::vsnprintf(format_buf_, sizeof(format_buf_), fmt, ap);
    if (len <= 0) {
        return;
    }

    size_t write_len = static_cast<size_t>(len);
    if (write_len >= sizeof(format_buf_)) {
        write_len = sizeof(format_buf_) - 1;
    }
    stream_->write(reinterpret_cast<const uint8_t *>(format_buf_), write_len);
}

void Shell::printf(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);
}

/* ================================================================== */
/*  Built-in commands                                                 */
/* ================================================================== */

void Shell::cmd_help() {
    puts("Commands:\r\n");
    for (const auto &command : BUILTIN_COMMANDS) {
        if (mode_ == Mode::ReadOnly && command.id == BuiltinId::Erase) {
            continue;
        }
        printf("  %-8s %s\r\n", command.name, command.help);
    }
    for (uint8_t i = 0; i < extra_count_; ++i) {
        printf("  %-8s %s\r\n", extra_cmds_[i].name, extra_cmds_[i].help);
    }
}

void Shell::run_builtin(BuiltinId id, int32_t argc, const char *const *argv) {
    switch (id) {
    case BuiltinId::Help: cmd_help(); break;
    case BuiltinId::List: cmd_ls(); break;
    case BuiltinId::Cat: cmd_cat(argc, argv); break;
    case BuiltinId::Erase: cmd_erase(argc, argv); break;
    }
}

/* ================================================================== */
/*  Line parsing & dispatch                                           */
/* ================================================================== */

void Shell::dispatch_line() {
    const char *args[MAX_ARGS] = {};
    int32_t argc = 0;

    char *p = line_buf_;
    while (*p != '\0' && argc < static_cast<int32_t>(MAX_ARGS)) {
        while (*p == ' ') {
            ++p;
        }
        if (*p == '\0') {
            break;
        }
        args[argc++] = p;
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

    for (const auto &command : BUILTIN_COMMANDS) {
        if (mode_ == Mode::ReadOnly && command.id == BuiltinId::Erase) {
            continue;
        }
        if (std::strcmp(args[0], command.name) == 0) {
            run_builtin(command.id, argc, args);
            return;
        }
    }

    for (uint8_t i = 0; i < extra_count_; ++i) {
        if (std::strcmp(args[0], extra_cmds_[i].name) == 0) {
            extra_cmds_[i].handler(*this, argc, args);
            return;
        }
    }

    printf("Unknown command: %s\r\n", args[0]);
}

/* ================================================================== */
/*  Shell methods                                                     */
/* ================================================================== */

void Shell::init(io::Stream &stream,
                 fs::FileSystem &file_system,
                 const Command *extra_cmds,
                 uint8_t extra_count,
                 Mode mode) {
    stream_ = &stream;
    file_system_ = &file_system;
    extra_cmds_ = extra_cmds;
    extra_count_ = extra_count;
    mode_ = mode;
    line_pos_ = 0;
    ignore_lf_ = false;
    puts(PROMPT);
}

void Shell::feed_char(char ch) {
    if (ch == '\r') {
        ignore_lf_ = true;
    } else if (ch == '\n') {
        if (ignore_lf_) {
            ignore_lf_ = false;
            return;
        }
        ignore_lf_ = false;
    } else {
        ignore_lf_ = false;
    }

    if (ch == '\r' || ch == '\n') {
        puts("\r\n");
        line_buf_[line_pos_] = '\0';
        dispatch_line();
        line_pos_ = 0;
        puts(PROMPT);
        return;
    }

    if (ch == '\b' || ch == 0x7F) {
        if (line_pos_ > 0) {
            --line_pos_;
            puts("\b \b");
        }
        return;
    }

    if (line_pos_ < LINE_BUF_SIZE - 1) {
        line_buf_[line_pos_++] = ch;
        if (stream_ != nullptr) {
            stream_->write(reinterpret_cast<const uint8_t *>(&ch), 1);
        }
    }
}

void Shell::poll() {
    if (stream_ == nullptr) {
        return;
    }

    int32_t len = stream_->read(rx_buf_, sizeof(rx_buf_), 0);
    if (len <= 0) {
        return;
    }
    for (int32_t i = 0; i < len; ++i) {
        feed_char(static_cast<char>(rx_buf_[i]));
    }
}

}  // namespace shell
