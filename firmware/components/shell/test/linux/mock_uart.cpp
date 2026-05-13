/**
 * @file mock_uart.cpp
 * @brief テスト用 io::Stream モック — 出力をバッファにキャプチャ
 */

#include "io_stream.h"

#include <cstring>
#include <cstdio>

/* ================================================================== */
/*  MockStream 実装                                                   */
/* ================================================================== */

class MockStream : public io::Stream {
  public:
    int32_t write(const uint8_t *data, size_t len) override {
        size_t space = sizeof(m_output_buf) - 1 - m_output_pos;
        size_t to_copy = (len < space) ? len : space;
        std::memcpy(m_output_buf + m_output_pos, data, to_copy);
        m_output_pos += to_copy;
        return static_cast<int32_t>(to_copy);
    }

    int32_t read(uint8_t * /*buf*/, size_t /*buf_len*/, uint32_t /*timeout_ms*/) override {
        return 0;
    }

    void reset() {
        m_output_pos = 0;
        m_output_buf[0] = '\0';
    }

    const char *get_output() {
        m_output_buf[m_output_pos] = '\0';
        return m_output_buf;
    }

    size_t get_output_len() const {
        return m_output_pos;
    }

  private:
    char m_output_buf[4096] = {};
    size_t m_output_pos = 0;
};

/* グローバルインスタンス (テスト全体で共有) */
static MockStream s_mock_stream;

/* ================================================================== */
/*  C リンケージヘルパ (既存テストとの互換)                            */
/* ================================================================== */

extern "C" void mock_uart_reset() {
    s_mock_stream.reset();
}

extern "C" const char *mock_uart_get_output() {
    return s_mock_stream.get_output();
}

extern "C" size_t mock_uart_get_output_len() {
    return s_mock_stream.get_output_len();
}

/* テストから MockStream インスタンスを取得する */
io::Stream &mock_get_stream() {
    return s_mock_stream;
}
