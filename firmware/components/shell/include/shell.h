#pragma once

/**
 * @file shell.h
 * @brief 軽量 UART シェル
 *
 * コマンドテーブル駆動のシンプルなシェル。
 * UART からの1行入力をパースし、登録コマンドを実行する。
 *
 * 組み込みコマンド: ls, cat, erase, help
 * 外部コマンドはテーブル登録で追加可能。
 */

#include <cstdint>
#include <cstddef>
#include <cstdarg>

#include "fs.h"

namespace io {
class Stream;
}

namespace shell {

/* ================================================================== */
/*  Types                                                             */
/* ================================================================== */

class Shell;

/** コマンドハンドラ関数型 */
using CmdHandler = void (*)(Shell &shell, int32_t argc, const char *const *argv);

/** シェルの操作モード */
enum class Mode : uint8_t {
    ReadWrite,
    ReadOnly,
};

/** コマンドエントリ */
struct Command {
    const char *name;   /**< コマンド名 */
    const char *help;   /**< ヘルプ文字列 (1行) */
    CmdHandler handler; /**< 実行関数 */
};

/**
 * @brief シェル本体
 *
 * 入力状態や出力先をインスタンスごとに保持する。
 */
class Shell {
  public:
    Shell() = default;

    void init(io::Stream &stream,
              fs::FileSystem &file_system,
              const Command *extra_cmds,
              uint8_t extra_count,
              Mode mode = Mode::ReadWrite);

    void feed_char(char ch);
    void poll();
    void puts(const char *str);
    void printf(const char *fmt, ...);

  private:
    static constexpr size_t LINE_BUF_SIZE = 128;
    static constexpr size_t RX_BUF_SIZE = 32;
    static constexpr size_t FILE_BUF_SIZE = 256;
    static constexpr size_t FORMAT_BUF_SIZE = 256;
    static constexpr size_t MAX_ARGS = 8;
    static constexpr char PROMPT[] = "> ";

    enum class BuiltinId : uint8_t {
        Help,
        List,
        Cat,
        Erase,
    };

    struct BuiltinCommand {
        const char *name;
        const char *help;
        BuiltinId id;
    };

    static constexpr BuiltinCommand BUILTIN_COMMANDS[] = {
        {"help", "Show available commands", BuiltinId::Help},
        {"ls", "List flash files", BuiltinId::List},
        {"cat", "cat <file> [--hex] - Read file", BuiltinId::Cat},
        {"erase", "erase <file> - Erase file", BuiltinId::Erase},
    };

    void dispatch_line();
    void vprintf(const char *fmt, va_list ap);
    void run_builtin(BuiltinId id, int32_t argc, const char *const *argv);
    void cmd_help();
    void cmd_ls();
    void cmd_cat(int32_t argc, const char *const *argv);
    void cmd_erase(int32_t argc, const char *const *argv);

    char line_buf_[LINE_BUF_SIZE];
    size_t line_pos_;
    bool ignore_lf_;
    uint8_t rx_buf_[RX_BUF_SIZE];
    uint8_t file_buf_[FILE_BUF_SIZE];
    char format_buf_[FORMAT_BUF_SIZE];

    const Command *extra_cmds_;
    uint8_t extra_count_;
    io::Stream *stream_;
    fs::FileSystem *file_system_;
    Mode mode_;
};

}  // namespace shell
