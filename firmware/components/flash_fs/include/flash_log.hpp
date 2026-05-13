#pragma once

/**
 * @file flash_log.hpp
 * @brief Flash ログ — 構造体直列化ベースのバイナリログ
 *
 * ログエントリは trivially_copyable な構造体として定義し、
 * 共通ヘッダ (RecordHeader) + payload として flash_fs に書き込む。
 *
 * デコードは tools/decode.py で行う。
 */

#include "flash_fs.h"

#include <cstdint>
#include <cstddef>
#include <type_traits>

namespace flash_log {

/* ================================================================== */
/*  Log Type                                                          */
/* ================================================================== */

enum class Type : uint8_t {
    Crash = 1,
    Event = 2,
    Ble = 3,
    Ota = 4,
};

/* ================================================================== */
/*  Record Header (common, 8 bytes)                                   */
/* ================================================================== */

struct RecordHeader {
    uint8_t version;       /**< レコードフォーマットバージョン */
    uint8_t type;          /**< Type enum 値 */
    uint8_t size;          /**< payload サイズ (bytes, max 255) */
    uint8_t reserved;      /**< 将来拡張 */
    uint16_t timestamp_ms; /**< uptime 下位16bit (ms) */
    uint16_t padding;      /**< 4byte align */
};

static_assert(sizeof(RecordHeader) == 8);

/* ================================================================== */
/*  Concept: FlashLogEntry                                            */
/* ================================================================== */

template <typename T>
concept FlashLogEntry = requires {
    { T::kType } -> std::convertible_to<Type>;
    requires std::is_trivially_copyable_v<T>;
    requires(sizeof(T) <= 255);
};

/* ================================================================== */
/*  Timestamp (プラットフォーム層で実装)                               */
/* ================================================================== */

uint16_t get_uptime_ms16();

/* ================================================================== */
/*  Write API                                                         */
/* ================================================================== */

static constexpr uint8_t RECORD_VERSION = 1;

template <FlashLogEntry T>
inline bool write(const T &entry) {
    RecordHeader hdr{};
    hdr.version = RECORD_VERSION;
    hdr.type = static_cast<uint8_t>(T::kType);
    hdr.size = static_cast<uint8_t>(sizeof(T));
    hdr.timestamp_ms = get_uptime_ms16();

    // ヘッダ + payload をまとめて flash_fs に追記
    // NOTE: 2回に分けるとページ境界で分断されるリスクがあるが
    //       flash_fs の Append はレコード単位で書くので問題ない
    uint8_t buf[sizeof(RecordHeader) + 255];
    __builtin_memcpy(buf, &hdr, sizeof(hdr));
    __builtin_memcpy(buf + sizeof(hdr), &entry, sizeof(T));

    return flash_fs::append(flash_fs::FILE_LOG, buf, sizeof(hdr) + sizeof(T));
}

/* ================================================================== */
/*  Log entry definitions                                             */
/* ================================================================== */

struct CrashEntry {
    static constexpr Type kType = Type::Crash;

    uint32_t fault_type;
    uint32_t pc;
    uint32_t lr;
    uint32_t sp;
    uint32_t stack_dump[8];
};

static_assert(std::is_trivially_copyable_v<CrashEntry>);

struct EventEntry {
    static constexpr Type kType = Type::Event;

    uint8_t event_id;
    uint8_t reserved[3];
    uint32_t param;
};

static_assert(std::is_trivially_copyable_v<EventEntry>);

struct BleEntry {
    static constexpr Type kType = Type::Ble;

    uint8_t addr[6]; /**< BLE address */
    int8_t rssi;
    uint8_t event_type; /**< adv type etc */
};

static_assert(std::is_trivially_copyable_v<BleEntry>);

struct OtaEntry {
    static constexpr Type kType = Type::Ota;

    uint8_t state; /**< OTAState */
    uint8_t reserved;
    uint16_t progress; /**< 0-1000 (0.1%単位) */
    uint32_t bytes;    /**< 受信バイト数 */
};

static_assert(std::is_trivially_copyable_v<OtaEntry>);

}  // namespace flash_log