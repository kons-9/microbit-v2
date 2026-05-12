/**
 * @file test_log_format.cpp
 * @brief log_vformat のユニットテスト
 */

#include <catch2/catch_test_macros.hpp>
#include <string>
#include "log.h"

/* mock_uart ヘルパ宣言 */
extern "C" void mock_uart_reset();
extern "C" const char *mock_uart_get_output();

/* LogOutput のプレフィックス "[I/TEST] " を除いた本文を取得（末尾改行除去） */
static std::string get_body() {
    const char *out = mock_uart_get_output();
    /* プレフィックスをスキップ: "[X/TAG] " */
    const char *space = out;
    while (*space && *space != ' ')
        space++;
    if (*space == ' ')
        space++;
    std::string body(space);
    /* 末尾改行を除去 */
    if (!body.empty() && body.back() == '\n')
        body.pop_back();
    return body;
}

#undef LOG_TAG
#define LOG_TAG "TEST"

TEST_CASE("log_vformat: basic %d", "[log]") {
    LogInit(LOG_LEVEL_DEBUG);
    mock_uart_reset();
    LOG_I("%d", 42);
    REQUIRE(std::string(get_body()) == "42");
}

TEST_CASE("log_vformat: negative %d", "[log]") {
    LogInit(LOG_LEVEL_DEBUG);
    mock_uart_reset();
    LOG_I("%d", -123);
    REQUIRE(std::string(get_body()) == "-123");
}

TEST_CASE("log_vformat: %u", "[log]") {
    LogInit(LOG_LEVEL_DEBUG);
    mock_uart_reset();
    LOG_I("%u", 4294967295u);
    REQUIRE(std::string(get_body()) == "4294967295");
}

TEST_CASE("log_vformat: %x lowercase", "[log]") {
    LogInit(LOG_LEVEL_DEBUG);
    mock_uart_reset();
    LOG_I("%x", 0xDEAD);
    REQUIRE(std::string(get_body()) == "dead");
}

TEST_CASE("log_vformat: %X uppercase", "[log]") {
    LogInit(LOG_LEVEL_DEBUG);
    mock_uart_reset();
    LOG_I("%X", 0xBEEF);
    REQUIRE(std::string(get_body()) == "BEEF");
}

TEST_CASE("log_vformat: %08x zero-padded", "[log]") {
    LogInit(LOG_LEVEL_DEBUG);
    mock_uart_reset();
    LOG_I("%08x", 0xAB);
    REQUIRE(std::string(get_body()) == "000000ab");
}

TEST_CASE("log_vformat: %ld positive", "[log]") {
    LogInit(LOG_LEVEL_DEBUG);
    mock_uart_reset();
    LOG_I("%ld", 123456L);
    REQUIRE(std::string(get_body()) == "123456");
}

TEST_CASE("log_vformat: %ld negative", "[log]") {
    LogInit(LOG_LEVEL_DEBUG);
    mock_uart_reset();
    LOG_I("%ld", -99999L);
    REQUIRE(std::string(get_body()) == "-99999");
}

TEST_CASE("log_vformat: %lu", "[log]") {
    LogInit(LOG_LEVEL_DEBUG);
    mock_uart_reset();
    LOG_I("%lu", 3000000000UL);
    REQUIRE(std::string(get_body()) == "3000000000");
}

TEST_CASE("log_vformat: %lx", "[log]") {
    LogInit(LOG_LEVEL_DEBUG);
    mock_uart_reset();
    LOG_I("%lx", 0xCAFEUL);
    REQUIRE(std::string(get_body()) == "cafe");
}

TEST_CASE("log_vformat: %lX", "[log]") {
    LogInit(LOG_LEVEL_DEBUG);
    mock_uart_reset();
    LOG_I("%lX", 0xF00DUL);
    REQUIRE(std::string(get_body()) == "F00D");
}

TEST_CASE("log_vformat: %s string", "[log]") {
    LogInit(LOG_LEVEL_DEBUG);
    mock_uart_reset();
    LOG_I("%s", "hello");
    REQUIRE(std::string(get_body()) == "hello");
}

TEST_CASE("log_vformat: %s NULL", "[log]") {
    LogInit(LOG_LEVEL_DEBUG);
    mock_uart_reset();
    LOG_I("%s", (const char *)nullptr);
    REQUIRE(std::string(get_body()) == "(null)");
}

TEST_CASE("log_vformat: %c", "[log]") {
    LogInit(LOG_LEVEL_DEBUG);
    mock_uart_reset();
    LOG_I("%c", 'A');
    REQUIRE(std::string(get_body()) == "A");
}

TEST_CASE("log_vformat: %%", "[log]") {
    LogInit(LOG_LEVEL_DEBUG);
    mock_uart_reset();
    LOG_I("100%%");
    REQUIRE(std::string(get_body()) == "100%");
}

TEST_CASE("log_vformat: mixed format", "[log]") {
    LogInit(LOG_LEVEL_DEBUG);
    mock_uart_reset();
    LOG_I("val=%ld cnt=%lu", -1L, 42UL);
    REQUIRE(std::string(get_body()) == "val=-1 cnt=42");
}
