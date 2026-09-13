/**
 * @file fs.cpp
 * @brief Flash FS コアロジック
 *
 * ページレイアウト (RingBuffer file):
 * ┌──────────────────────────────┐
 * │  PageHeader (4 bytes)        │
 * │    sequence: uint32_t        │  ← ページ書き込み順 (ring 特定用)
 * ├──────────────────────────────┤
 * │  Data records...             │
 * │  (消去状態=0xFF で終端検出)    │
 * └──────────────────────────────┘
 *
 * ページレイアウト (Fixed file):
 * ┌──────────────────────────────┐
 * │  Raw data (user manages)     │
 * └──────────────────────────────┘
 *
 * write_offset は Flash に保存せず、起動時にページ末尾スキャンで復元する。
 * (NOR Flash では上書き更新ができないため)
 */

#include "fs.h"
#include <sysconfig/flash_layout.h>

#include <cstring>

namespace fs {

/* ================================================================== */
/*  Constants                                                         */
/* ================================================================== */

static constexpr uint32_t PAGE_SIZE = 4096;

static constexpr uint32_t PAGE_HEADER_SIZE = 4; /* sequence のみ */
static constexpr uint32_t ERASED_WORD = 0xFFFFFFFF;

constexpr size_t file_index(FileId id) {
    return static_cast<size_t>(id);
}

/* ================================================================== */
/*  File table (compile-time configuration)                           */
/* ================================================================== */

struct FileTableEntry {
    FileId id;
    FileType type;
    const char *name;
    uint32_t start_address;
    uint8_t page_count;
};

/* ================================================================== */
/*  File table address resolution                                     */
/* ================================================================== */

static constexpr FileTableEntry FILE_TABLE[] = {
    {FileId::Log, FileType::RingBuffer, "log", 0, 4},       // 16KB
    {FileId::Settings, FileType::Fixed, "settings", 0, 1},  //  4KB
    {FileId::Calib, FileType::Fixed, "calib", 0, 1},        //  4KB
};

static_assert(sizeof(FILE_TABLE) / sizeof(FILE_TABLE[0]) == static_cast<size_t>(FileId::Count));

/* ================================================================== */
/*  Runtime state                                                     */
/* ================================================================== */

struct PageHeader {
    uint32_t sequence; /**< ページ使用順 (ring buffer 位置特定用) */
};

/* ================================================================== */
/*  Internal helpers                                                  */
/* ================================================================== */

uint32_t FileSystem::get_file_base_address(FileId id) const {
    return base_address_[file_index(id)];
}

uint32_t FileSystem::get_page_address(FileId id, uint32_t page_index) const {
    return get_file_base_address(id) + page_index * PAGE_SIZE;
}

void FileSystem::read_page_header(uint32_t page_addr, void *header) {
    flash_.read(page_addr, header, sizeof(PageHeader));
}

void FileSystem::write_page_header(uint32_t page_addr, const void *header) {
    flash_.write(page_addr, header, sizeof(PageHeader));
}


/**
 * @brief ページ内の書き込み末尾位置をスキャンで求める
 *
 * ヘッダ直後からワード単位で読み、最初の 0xFFFFFFFF ワードを見つけたらそこが末尾。
 */
uint32_t FileSystem::scan_write_offset(uint32_t page_addr) {
    uint32_t offset = PAGE_HEADER_SIZE;
    while (offset < PAGE_SIZE) {
        uint32_t word;
        flash_.read(page_addr + offset, &word, sizeof(word));
        if (word == ERASED_WORD) {
            break;
        }
        offset += sizeof(uint32_t);
    }
    return offset;
}

void FileSystem::init_stream_state(FileId id) {
    const auto &entry = FILE_TABLE[file_index(id)];
    auto &ss = stream_[file_index(id)];

    // 全ページをスキャンし、最大シーケンス番号のページを見つける
    uint32_t max_seq = 0;
    uint32_t max_page = 0;
    bool found = false;

    for (uint8_t p = 0; p < entry.page_count; ++p) {
        uint32_t addr = get_page_address(id, p);
        PageHeader hdr;
        read_page_header(addr, &hdr);

        if (hdr.sequence != ERASED_WORD) {
            if (!found || hdr.sequence > max_seq) {
                max_seq = hdr.sequence;
                max_page = p;
                found = true;
            }
        }
    }

    if (found) {
        ss.current_page = max_page;
        ss.sequence = max_seq + 1;

        // write_offset をスキャンで復元
        uint32_t addr = get_page_address(id, max_page);
        ss.write_offset = scan_write_offset(addr);
    } else {
        // 全ページ未使用 — ページ0から開始, ヘッダを書いておく
        ss.current_page = 0;
        ss.sequence = 1;
        ss.write_offset = PAGE_HEADER_SIZE;

        uint32_t addr = get_page_address(id, 0);
        PageHeader hdr;
        hdr.sequence = ss.sequence++;
        write_page_header(addr, &hdr);
    }
}

void FileSystem::advance_to_next_page(FileId id) {
    const auto &entry = FILE_TABLE[file_index(id)];
    auto &ss = stream_[file_index(id)];

    uint32_t next_page = (ss.current_page + 1) % entry.page_count;
    uint32_t next_addr = get_page_address(id, next_page);

    // 次ページを消去 (ring: 最古データを破棄)
    flash_.page_erase(next_addr);

    // 新しいページヘッダを書く
    PageHeader hdr;
    hdr.sequence = ss.sequence++;
    write_page_header(next_addr, &hdr);

    ss.current_page = next_page;
    ss.write_offset = PAGE_HEADER_SIZE;
}

/* ================================================================== */
/*  Public API                                                        */
/* ================================================================== */

FileSystem::FileSystem(drivers::Flash &flash)
    : flash_(flash) {
}

int32_t FileSystem::init() {
    // ベースアドレスを設定
    base_address_[file_index(FileId::Log)] = sysconfig::flash_layout::log_base_address();
    base_address_[file_index(FileId::Settings)] = sysconfig::flash_layout::settings_base_address();
    base_address_[file_index(FileId::Calib)] = sysconfig::flash_layout::calib_base_address();

    // Stream ファイルの状態を復元
    for (size_t i = 0; i < static_cast<size_t>(FileId::Count); ++i) {
        if (FILE_TABLE[i].type == FileType::RingBuffer) {
            init_stream_state(static_cast<FileId>(i));
        }
    }

    initialized_ = true;
    return 0;
}

library::Option<const char *> FileSystem::get_name(FileId id) {
    if (file_index(id) >= static_cast<size_t>(FileId::Count)) {
        return library::Option<const char *>::none();
    }
    return library::Option<const char *>::some(FILE_TABLE[file_index(id)].name);
}

library::Option<FileId> FileSystem::find_by_name(const char *name) {
    if (name == nullptr) {
        return library::Option<FileId>::none();
    }
    for (size_t i = 0; i < static_cast<size_t>(FileId::Count); ++i) {
        if (std::strcmp(FILE_TABLE[i].name, name) == 0) {
            return library::Option<FileId>::some(static_cast<FileId>(i));
        }
    }
    return library::Option<FileId>::none();
}

/* --- Stream --- */

library::Result<void, Error> FileSystem::append(FileId id, const void *data, size_t size) {
    if (file_index(id) >= static_cast<size_t>(FileId::Count)) {
        return library::Result<void, Error>::err(Error::InvalidFile);
    }
    if (FILE_TABLE[file_index(id)].type != FileType::RingBuffer) {
        return library::Result<void, Error>::err(Error::WrongFileType);
    }
    if (size == 0 || size > (PAGE_SIZE - PAGE_HEADER_SIZE)) {
        return library::Result<void, Error>::err(Error::TooLarge);
    }

    auto &ss = stream_[file_index(id)];

    // 現在ページに入りきらなければ次ページへ
    if (ss.write_offset + size > PAGE_SIZE) {
        advance_to_next_page(id);
    }

    // データ書き込み
    uint32_t addr = get_page_address(id, ss.current_page) + ss.write_offset;
    flash_.write(addr, data, size);
    ss.write_offset += static_cast<uint32_t>(size);

    // 4バイトアライン
    ss.write_offset = (ss.write_offset + 3) & ~3u;

    return library::Result<void, Error>::ok();
}

size_t FileSystem::read(FileId id, uint32_t offset, void *buf, size_t size) {
    if (file_index(id) >= static_cast<size_t>(FileId::Count)
        || FILE_TABLE[file_index(id)].type != FileType::RingBuffer) {
        return 0;
    }

    const auto &entry = FILE_TABLE[file_index(id)];
    uint32_t total_data_size = entry.page_count * (PAGE_SIZE - PAGE_HEADER_SIZE);
    if (offset >= total_data_size) {
        return 0;
    }

    size_t remaining = size;
    auto *dst = static_cast<uint8_t *>(buf);

    // sequence の昇順、つまり最古ページから順に読む。
    // 物理ページ番号とring bufferの論理順序は一致しない。
    uint32_t byte_pos = 0;
    bool has_last_sequence = false;
    uint32_t last_sequence = 0;
    for (uint8_t order = 0; order < entry.page_count && remaining > 0; ++order) {
        bool found_page = false;
        uint8_t selected_page = 0;
        uint32_t selected_sequence = 0;

        for (uint8_t p = 0; p < entry.page_count; ++p) {
            PageHeader hdr;
            read_page_header(get_page_address(id, p), &hdr);
            if (hdr.sequence == ERASED_WORD || (has_last_sequence && hdr.sequence <= last_sequence)) {
                continue;
            }
            if (!found_page || hdr.sequence < selected_sequence) {
                found_page = true;
                selected_page = p;
                selected_sequence = hdr.sequence;
            }
        }

        if (!found_page) {
            break;
        }

        has_last_sequence = true;
        last_sequence = selected_sequence;

        uint32_t page_addr = get_page_address(id, selected_page);
        uint32_t page_write_end = scan_write_offset(page_addr);
        uint32_t data_len = page_write_end - PAGE_HEADER_SIZE;

        if (byte_pos + data_len <= offset) {
            byte_pos += data_len;
            continue;
        }

        uint32_t skip = (offset > byte_pos) ? (offset - byte_pos) : 0;
        uint32_t to_read = data_len - skip;
        if (to_read > remaining) {
            to_read = static_cast<uint32_t>(remaining);
        }

        flash_.read(page_addr + PAGE_HEADER_SIZE + skip, dst, to_read);
        dst += to_read;
        remaining -= to_read;
        byte_pos += data_len;
    }

    return size - remaining;
}

/* --- Block --- */

library::Result<void, Error> FileSystem::block_write(FileId id, uint32_t offset, const void *data, size_t size) {
    if (file_index(id) >= static_cast<size_t>(FileId::Count)) {
        return library::Result<void, Error>::err(Error::InvalidFile);
    }
    if (FILE_TABLE[file_index(id)].type != FileType::Fixed) {
        return library::Result<void, Error>::err(Error::WrongFileType);
    }

    const auto &entry = FILE_TABLE[file_index(id)];
    uint32_t capacity = entry.page_count * PAGE_SIZE;
    if (offset + size > capacity) {
        return library::Result<void, Error>::err(Error::OutOfBounds);
    }

    uint32_t addr = get_file_base_address(id) + offset;
    flash_.write(addr, data, size);
    return library::Result<void, Error>::ok();
}

size_t FileSystem::block_read(FileId id, uint32_t offset, void *buf, size_t size) {
    if (file_index(id) >= static_cast<size_t>(FileId::Count) || FILE_TABLE[file_index(id)].type != FileType::Fixed) {
        return 0;
    }

    const auto &entry = FILE_TABLE[file_index(id)];
    uint32_t capacity = entry.page_count * PAGE_SIZE;
    if (offset >= capacity) {
        return 0;
    }

    size_t to_read = size;
    if (offset + to_read > capacity) {
        to_read = capacity - offset;
    }

    flash_.read(get_file_base_address(id) + offset, buf, to_read);
    return to_read;
}

/* --- 共通 --- */

void FileSystem::erase(FileId id) {
    if (file_index(id) >= static_cast<size_t>(FileId::Count)) {
        return;
    }

    const auto &entry = FILE_TABLE[file_index(id)];
    for (uint8_t p = 0; p < entry.page_count; ++p) {
        flash_.page_erase(get_page_address(id, p));
    }

    // Stream状態をリセット
    if (entry.type == FileType::RingBuffer) {
        auto &ss = stream_[file_index(id)];
        ss.current_page = 0;
        ss.write_offset = PAGE_HEADER_SIZE;
        ss.sequence = 1;

        // ページ0にヘッダを書く
        uint32_t addr = get_page_address(id, 0);
        PageHeader hdr;
        hdr.sequence = ss.sequence++;
        write_page_header(addr, &hdr);
    }
}

library::Result<void, Error> FileSystem::get_info(FileId id, FileInfo *info) {
    if (file_index(id) >= static_cast<size_t>(FileId::Count)) {
        return library::Result<void, Error>::err(Error::InvalidFile);
    }
    if (info == nullptr) {
        return library::Result<void, Error>::err(Error::InvalidArgument);
    }

    const auto &entry = FILE_TABLE[file_index(id)];
    info->id = entry.id;
    info->type = entry.type;
    info->name = entry.name;
    info->capacity = entry.page_count * PAGE_SIZE;

    if (entry.type == FileType::RingBuffer) {
        // 使用量: 全有効ページの write_offset 合計
        uint32_t used = 0;
        for (uint8_t p = 0; p < entry.page_count; ++p) {
            uint32_t addr = get_page_address(id, p);
            PageHeader hdr;
            read_page_header(addr, &hdr);
            if (hdr.sequence != ERASED_WORD) {
                used += scan_write_offset(addr) - PAGE_HEADER_SIZE;
            }
        }
        info->used = used;
    } else {
        // Block: 使用量は外部管理 (ここではcapacity扱い)
        info->used = info->capacity;
    }

    return library::Result<void, Error>::ok();
}

/* ================================================================== */
/* Stream adapters                                                    */
/* ================================================================== */

RingBufferFile::RingBufferFile(FileSystem &file_system, FileId id)
    : file_system_(file_system)
    , id_(id) {
}

int32_t RingBufferFile::write(const uint8_t *data, size_t len) {
    if (data == nullptr || len == 0) {
        return 0;
    }

    auto result = file_system_.append(id_, data, len);
    return result ? static_cast<int32_t>(len) : -1;
}

int32_t RingBufferFile::read(uint8_t *buf, size_t buf_len, uint32_t /*timeout_ms*/) {
    if (buf == nullptr || buf_len == 0) {
        return 0;
    }

    FileInfo info;
    if (!file_system_.get_info(id_, &info) || read_offset_ >= info.used) {
        return 0;
    }

    size_t read_size = buf_len;
    if (read_offset_ + read_size > info.used) {
        read_size = info.used - read_offset_;
    }

    size_t read_count = file_system_.read(id_, read_offset_, buf, read_size);
    read_offset_ += static_cast<uint32_t>(read_count);
    return static_cast<int32_t>(read_count);
}

void RingBufferFile::rewind() {
    read_offset_ = 0;
}

FixedFile::FixedFile(FileSystem &file_system, FileId id)
    : file_system_(file_system)
    , id_(id) {
}

int32_t FixedFile::write(const uint8_t *data, size_t len) {
    if (data == nullptr || len == 0) {
        return 0;
    }

    auto result = file_system_.block_write(id_, write_offset_, data, len);
    if (!result) {
        return -1;
    }

    write_offset_ += static_cast<uint32_t>(len);
    return static_cast<int32_t>(len);
}

int32_t FixedFile::read(uint8_t *buf, size_t buf_len, uint32_t /*timeout_ms*/) {
    if (buf == nullptr || buf_len == 0) {
        return 0;
    }

    size_t read_count = file_system_.block_read(id_, read_offset_, buf, buf_len);
    read_offset_ += static_cast<uint32_t>(read_count);
    return static_cast<int32_t>(read_count);
}

void FixedFile::rewind_read() {
    read_offset_ = 0;
}

void FixedFile::rewind_write() {
    write_offset_ = 0;
}

}  // namespace fs
