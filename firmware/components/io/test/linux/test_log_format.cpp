/**
 * @file test_log_format.cpp
 * @brief log_vformat のユニットテスト
 */

#include <catch2/catch_test_macros.hpp>
#include <string>
#include "log.h"

/* mock helpers */
extern "C" void mock_uart_reset();
extern "C" const char *mock_uart_get_output();

namespace io {
class Stream;
}
extern io::Stream &mock_get_stream();

class SecondaryStream final : public io::Stream {
  public:
    int32_t write(const uint8_t *data, size_t len) override {
        output_.append(reinterpret_cast<const char *>(data), len);
        return static_cast<int32_t>(len);
    }

    int32_t read(uint8_t * /*buf*/, size_t /*buf_len*/, uint32_t /*timeout_ms*/) override {
        return 0;
    }

    void reset() {
        output_.clear();
    }

    const std::string &output() const {
        return output_;
    }

  private:
    std::string output_;
};

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
    logging::Logger::instance().init(logging::LogLevel::Debug, mock_get_stream());
    mock_uart_reset();
    LOG_I("%d", 42);
    REQUIRE(std::string(get_body()) == "42");
}

TEST_CASE("log_vformat: negative %d", "[log]") {
    logging::Logger::instance().init(logging::LogLevel::Debug, mock_get_stream());
    mock_uart_reset();
    LOG_I("%d", -123);
    REQUIRE(std::string(get_body()) == "-123");
}

TEST_CASE("log_vformat: %u", "[log]") {
    logging::Logger::instance().init(logging::LogLevel::Debug, mock_get_stream());
    mock_uart_reset();
    LOG_I("%u", 4294967295u);
    REQUIRE(std::string(get_body()) == "4294967295");
}

TEST_CASE("log_vformat: %x lowercase", "[log]") {
    logging::Logger::instance().init(logging::LogLevel::Debug, mock_get_stream());
    mock_uart_reset();
    LOG_I("%x", 0xDEAD);
    REQUIRE(std::string(get_body()) == "dead");
}

TEST_CASE("log_vformat: %X uppercase", "[log]") {
    logging::Logger::instance().init(logging::LogLevel::Debug, mock_get_stream());
    mock_uart_reset();
    LOG_I("%X", 0xBEEF);
    REQUIRE(std::string(get_body()) == "BEEF");
}

TEST_CASE("log_vformat: %08x zero-padded", "[log]") {
    logging::Logger::instance().init(logging::LogLevel::Debug, mock_get_stream());
    mock_uart_reset();
    LOG_I("%08x", 0xAB);
    REQUIRE(std::string(get_body()) == "000000ab");
}

TEST_CASE("log_vformat: %ld positive", "[log]") {
    logging::Logger::instance().init(logging::LogLevel::Debug, mock_get_stream());
    mock_uart_reset();
    LOG_I("%ld", 123456L);
    REQUIRE(std::string(get_body()) == "123456");
}

TEST_CASE("log_vformat: %ld negative", "[log]") {
    logging::Logger::instance().init(logging::LogLevel::Debug, mock_get_stream());
    mock_uart_reset();
    LOG_I("%ld", -99999L);
    REQUIRE(std::string(get_body()) == "-99999");
}

TEST_CASE("log_vformat: %lu", "[log]") {
    logging::Logger::instance().init(logging::LogLevel::Debug, mock_get_stream());
    mock_uart_reset();
    LOG_I("%lu", 3000000000UL);
    REQUIRE(std::string(get_body()) == "3000000000");
}

TEST_CASE("log_vformat: %lx", "[log]") {
    logging::Logger::instance().init(logging::LogLevel::Debug, mock_get_stream());
    mock_uart_reset();
    LOG_I("%lx", 0xCAFEUL);
    REQUIRE(std::string(get_body()) == "cafe");
}

TEST_CASE("log_vformat: %lX", "[log]") {
    logging::Logger::instance().init(logging::LogLevel::Debug, mock_get_stream());
    mock_uart_reset();
    LOG_I("%lX", 0xF00DUL);
    REQUIRE(std::string(get_body()) == "F00D");
}

TEST_CASE("log_vformat: %s string", "[log]") {
    logging::Logger::instance().init(logging::LogLevel::Debug, mock_get_stream());
    mock_uart_reset();
    LOG_I("%s", "hello");
    REQUIRE(std::string(get_body()) == "hello");
}

TEST_CASE("log_vformat: %s NULL", "[log]") {
    logging::Logger::instance().init(logging::LogLevel::Debug, mock_get_stream());
    mock_uart_reset();
    LOG_I("%s", (const char *)nullptr);
    REQUIRE(std::string(get_body()) == "(null)");
}

TEST_CASE("log_vformat: %c", "[log]") {
    logging::Logger::instance().init(logging::LogLevel::Debug, mock_get_stream());
    mock_uart_reset();
    LOG_I("%c", 'A');
    REQUIRE(std::string(get_body()) == "A");
}

TEST_CASE("log_vformat: %%", "[log]") {
    logging::Logger::instance().init(logging::LogLevel::Debug, mock_get_stream());
    mock_uart_reset();
    LOG_I("100%%");
    REQUIRE(std::string(get_body()) == "100%");
}

TEST_CASE("log_vformat: mixed format", "[log]") {
    logging::Logger::instance().init(logging::LogLevel::Debug, mock_get_stream());
    mock_uart_reset();
    LOG_I("val=%ld cnt=%lu", -1L, 42UL);
    REQUIRE(std::string(get_body()) == "val=-1 cnt=42");
}

TEST_CASE("logger writes to multiple streams", "[log]") {
    SecondaryStream secondary;
    logging::Logger::instance().init(logging::LogLevel::Debug, mock_get_stream());
    REQUIRE(logging::Logger::instance().add_stream(secondary));

    mock_uart_reset();
    secondary.reset();
    LOG_I("fanout %u", 42u);

    REQUIRE(std::string(mock_uart_get_output()) == secondary.output());
}
