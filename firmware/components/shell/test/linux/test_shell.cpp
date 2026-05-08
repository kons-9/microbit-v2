#include <catch2/catch_test_macros.hpp>
#include "shell.h"
#include "flash_fs.h"

#include <cstring>

/* Mock UART helpers */
extern "C" void mock_uart_reset();
extern "C" const char *mock_uart_get_output();
extern "C" size_t mock_uart_get_output_len();

/* Flash テスト用リセット */
extern "C" void flash_fs_arch_test_reset();

struct ShellFixture {
    ShellFixture() {
        flash_fs_arch_test_reset();
        flash_fs_init();
        mock_uart_reset();
        shell_init(nullptr, 0);
        mock_uart_reset();  // Init時のプロンプト出力をクリア
    }

    /* 文字列をShellに1文字ずつ投入し、最後にEnter */
    void feed_line(const char *line) {
        mock_uart_reset();
        for (const char *p = line; *p; ++p) {
            shell_feed_char(*p);
        }
        shell_feed_char('\r');
    }
};

/* ================================================================== */
/*  Line parsing                                                      */
/* ================================================================== */

TEST_CASE_METHOD(ShellFixture, "Empty line prints prompt only", "[shell]") {
    feed_line("");
    const char *out = mock_uart_get_output();
    // 空行 → "\r\n> " (改行+プロンプト)
    REQUIRE(std::strstr(out, "> ") != nullptr);
}

TEST_CASE_METHOD(ShellFixture, "Unknown command error message", "[shell]") {
    feed_line("foobar");
    const char *out = mock_uart_get_output();
    REQUIRE(std::strstr(out, "Unknown command: foobar") != nullptr);
}

/* ================================================================== */
/*  Built-in commands                                                 */
/* ================================================================== */

TEST_CASE_METHOD(ShellFixture, "help lists commands", "[shell]") {
    feed_line("help");
    const char *out = mock_uart_get_output();
    REQUIRE(std::strstr(out, "help") != nullptr);
    REQUIRE(std::strstr(out, "ls") != nullptr);
    REQUIRE(std::strstr(out, "cat") != nullptr);
    REQUIRE(std::strstr(out, "erase") != nullptr);
}

TEST_CASE_METHOD(ShellFixture, "ls shows flash files", "[shell]") {
    feed_line("ls");
    const char *out = mock_uart_get_output();
    REQUIRE(std::strstr(out, "log") != nullptr);
    REQUIRE(std::strstr(out, "settings") != nullptr);
    REQUIRE(std::strstr(out, "calib") != nullptr);
    REQUIRE(std::strstr(out, "stream") != nullptr);
    REQUIRE(std::strstr(out, "block") != nullptr);
}

TEST_CASE_METHOD(ShellFixture, "erase known file prints OK", "[shell]") {
    feed_line("erase log");
    const char *out = mock_uart_get_output();
    REQUIRE(std::strstr(out, "OK") != nullptr);
}

TEST_CASE_METHOD(ShellFixture, "erase unknown file prints error", "[shell]") {
    feed_line("erase nonexist");
    const char *out = mock_uart_get_output();
    REQUIRE(std::strstr(out, "Unknown file") != nullptr);
}

TEST_CASE_METHOD(ShellFixture, "cat without args prints usage", "[shell]") {
    feed_line("cat");
    const char *out = mock_uart_get_output();
    REQUIRE(std::strstr(out, "Usage") != nullptr);
}

/* ================================================================== */
/*  Extra commands                                                    */
/* ================================================================== */

static bool s_customCalled = false;
static int32_t s_customArgc = 0;

static void custom_handler(int32_t argc, const char *const * /*argv*/) {
    s_customCalled = true;
    s_customArgc = argc;
}

TEST_CASE("shell_init with extra commands", "[shell]") {
    flash_fs_arch_test_reset();
    flash_fs_init();
    mock_uart_reset();

    static const ShellCommand extra[] = {
        {"mycmd", "My test command", custom_handler},
    };
    shell_init(extra, 1);
    mock_uart_reset();

    s_customCalled = false;
    s_customArgc = 0;

    const char *line = "mycmd arg1 arg2\r";
    for (const char *p = line; *p; ++p) {
        shell_feed_char(*p);
    }

    REQUIRE(s_customCalled);
    REQUIRE(s_customArgc == 3);
}

/* ================================================================== */
/*  Backspace handling                                                */
/* ================================================================== */

TEST_CASE_METHOD(ShellFixture, "Backspace removes character", "[shell]") {
    // Type "hXlp" then backspace+backspace+"elp" → "help"
    mock_uart_reset();
    shell_feed_char('h');
    shell_feed_char('X');
    shell_feed_char('\b');  // delete 'X'
    shell_feed_char('e');
    shell_feed_char('l');
    shell_feed_char('p');
    shell_feed_char('\r');

    const char *out = mock_uart_get_output();
    // Should execute "help" successfully (shows command list)
    REQUIRE(std::strstr(out, "Commands:") != nullptr);
}
