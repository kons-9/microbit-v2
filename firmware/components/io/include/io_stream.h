#pragma once

/**
 * @file io_stream.h
 * @brief バイトストリーム抽象インターフェース
 *
 * log, shell などのコンポーネントが出力先に依存しないための契約。
 * drivers::Uart がこれを実装し、apps 層で注入する。
 */

#include <cstdint>
#include <cstddef>

namespace io {

class Stream {
  public:
    virtual ~Stream() = default;

    /**
     * データを送信する
     * @param data  送信データ
     * @param len   データ長 [bytes]
     * @return 送信バイト数, 負値はエラー
     */
    virtual int32_t write(const uint8_t *data, size_t len) = 0;

    /**
     * データを受信する (ブロッキング)
     * @param buf         受信バッファ
     * @param buf_len     バッファサイズ
     * @param timeout_ms  タイムアウト [ms] (0 = ノンブロッキング)
     * @return 受信バイト数, 負値はエラー
     */
    virtual int32_t read(uint8_t *buf, size_t buf_len, uint32_t timeout_ms) = 0;
};

}  // namespace io
