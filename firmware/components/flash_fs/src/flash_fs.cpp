/**
 * @file flash_fs.cpp
 * @brief Flash FS コアロジック
 *
 * ページレイアウト (Stream mode):
 * ┌──────────────────────────────┐
 * │  PageHeader (4 bytes)        │
 * │    sequence: uint32_t        │  ← ページ書き込み順 (ring 特定用)
 * ├──────────────────────────────┤
 * │  Data records...             │
 * │  (消去状態=0xFF で終端検出)    │
 * └──────────────────────────────┘
 *
 * ページレイアウト (Block mode):
 * ┌──────────────────────────────┐
 * │  Raw data (user manages)     │
 * └──────────────────────────────┘
 *
 * write_offset は Flash に保存せず、起動時にページ末尾スキャンで復元する。
 * (NOR Flash では上書き更新ができないため)
 */

#include "flash_fs.h"
#include "arch/flash_fs_arch.h"

#include <cstring>

/* ================================================================== */
/*  Constants                                                         */
/* ================================================================== */

static constexpr uint32_t PAGE_SIZE = 4096;

static constexpr uint32_t PAGE_HEADER_SIZE = 4; /* sequence のみ */
static constexpr uint32_t ERASED_WORD = 0xFFFFFFFF;

/* ================================================================== */
/*  File table (compile-time configuration)                           */
/* ================================================================== */

struct FileTableEntry {
    FlashFsFileId id;
    FlashFsMode mode;
    const char *name;
    uint32_t start_address;
    uint8_t page_count;
};

/* ================================================================== */
/*  File table address resolution                                     */
/* ================================================================== */

#if defined(FLASH_FS_ARCH_LINUX)
/* テスト用: 固定アドレス (Linux RAMエミュのベースに対応) */
static constexpr uint32_t LOG_BASE_ADDR = 0x70000;
static constexpr uint32_t SETTINGS_BASE_ADDR = 0x74000;
static constexpr uint32_t CALIB_BASE_ADDR = 0x75000;
#else
/* 実機: リンカシンボルから取得 */
extern "C" const uint32_t __flash_fs_log_start[];
extern "C" const uint32_t __flash_fs_settings_start[];
extern "C" const uint32_t __flash_fs_calib_start[];
static const uint32_t LOG_BASE_ADDR = reinterpret_cast<uint32_t>(__flash_fs_log_start);
static const uint32_t SETTINGS_BASE_ADDR = reinterpret_cast<uint32_t>(__flash_fs_settings_start);
static const uint32_t CALIB_BASE_ADDR = reinterpret_cast<uint32_t>(__flash_fs_calib_start);
#endif

static constexpr FileTableEntry FILE_TABLE[] = {
    {FLASH_FS_FILE_LOG, FLASH_FS_MODE_STREAM, "log", 0, 4},           // 16KB
    {FLASH_FS_FILE_SETTINGS, FLASH_FS_MODE_BLOCK, "settings", 0, 1},  //  4KB
    {FLASH_FS_FILE_CALIB, FLASH_FS_MODE_BLOCK, "calib", 0, 1},        //  4KB
};

static_assert(sizeof(FILE_TABLE) / sizeof(FILE_TABLE[0]) == FLASH_FS_FILE_COUNT);

/* ================================================================== */
/*  Runtime state                                                     */
/* ================================================================== */

struct PageHeader {
    uint32_t sequence; /**< ページ使用順 (ring buffer 位置特定用) */
};

struct StreamState {
    uint32_t current_page; /**< 現在書き込み中のページインデックス (0-based, file内) */
    uint32_t write_offset; /**< 現在ページ内の書き込み位置 */
    uint32_t sequence;     /**< 次に使うシーケンス番号 */
};

static struct {
    uint32_t base_address[FLASH_FS_FILE_COUNT];
    StreamState stream[FLASH_FS_FILE_COUNT];
    bool initialized;
} s_state;

/* ================================================================== */
/*  Internal helpers                                                  */
/* ================================================================== */

static uint32_t get_file_base_address(FlashFsFileId id) {
    return s_state.base_address[id];
}

static uint32_t get_page_address(FlashFsFileId id, uint32_t page_index) {
    return get_file_base_address(id) + page_index * PAGE_SIZE;
}

static void read_page_header(uint32_t page_addr, PageHeader *hdr) {
    flash_fs_arch_read(page_addr, hdr, sizeof(PageHeader));
}

static void write_page_header(uint32_t page_addr, const PageHeader *hdr) {
    flash_fs_arch_write(page_addr, hdr, sizeof(PageHeader));
}

static bool is_page_erased(uint32_t page_addr) {
    PageHeader hdr;
    read_page_header(page_addr, &hdr);
    return hdr.sequence == ERASED_WORD;
}

/**
 * @brief ページ内の書き込み末尾位置をスキャンで求める
 *
 * ヘッダ直後からワード単位で読み、最初の 0xFFFFFFFF ワードを見つけたらそこが末尾。
 */
static uint32_t scan_write_offset(uint32_t page_addr) {
    uint32_t offset = PAGE_HEADER_SIZE;
    while (offset < PAGE_SIZE) {
        uint32_t word;
        flash_fs_arch_read(page_addr + offset, &word, sizeof(word));
        if (word == ERASED_WORD) {
            break;
        }
        offset += sizeof(uint32_t);
    }
    return offset;
}

static void init_stream_state(FlashFsFileId id) {
    const auto &entry = FILE_TABLE[id];
    auto &ss = s_state.stream[id];

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

static void advance_to_next_page(FlashFsFileId id) {
    const auto &entry = FILE_TABLE[id];
    auto &ss = s_state.stream[id];

    uint32_t next_page = (ss.current_page + 1) % entry.page_count;
    uint32_t next_addr = get_page_address(id, next_page);

    // 次ページを消去 (ring: 最古データを破棄)
    flash_fs_arch_page_erase(next_addr);

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

int32_t flash_fs_init(void) {
    // ベースアドレスを設定
    s_state.base_address[FLASH_FS_FILE_LOG] = LOG_BASE_ADDR;
    s_state.base_address[FLASH_FS_FILE_SETTINGS] = SETTINGS_BASE_ADDR;
    s_state.base_address[FLASH_FS_FILE_CALIB] = CALIB_BASE_ADDR;

    // Stream ファイルの状態を復元
    for (uint8_t i = 0; i < FLASH_FS_FILE_COUNT; ++i) {
        if (FILE_TABLE[i].mode == FLASH_FS_MODE_STREAM) {
            init_stream_state(static_cast<FlashFsFileId>(i));
        }
    }

    s_state.initialized = true;
    return 0;
}

const char *flash_fs_get_name(FlashFsFileId id) {
    if (id >= FLASH_FS_FILE_COUNT) {
        return nullptr;
    }
    return FILE_TABLE[id].name;
}

FlashFsFileId flash_fs_find_by_name(const char *name) {
    for (uint8_t i = 0; i < FLASH_FS_FILE_COUNT; ++i) {
        if (std::strcmp(FILE_TABLE[i].name, name) == 0) {
            return static_cast<FlashFsFileId>(i);
        }
    }
    return FLASH_FS_FILE_COUNT;
}

/* --- Stream --- */

bool flash_fs_append(FlashFsFileId id, const void *data, size_t size) {
    if (id >= FLASH_FS_FILE_COUNT || FILE_TABLE[id].mode != FLASH_FS_MODE_STREAM) {
        return false;
    }
    if (size == 0 || size > (PAGE_SIZE - PAGE_HEADER_SIZE)) {
        return false;
    }

    auto &ss = s_state.stream[id];

    // 現在ページに入りきらなければ次ページへ
    if (ss.write_offset + size > PAGE_SIZE) {
        advance_to_next_page(id);
    }

    // データ書き込み
    uint32_t addr = get_page_address(id, ss.current_page) + ss.write_offset;
    flash_fs_arch_write(addr, data, size);
    ss.write_offset += static_cast<uint32_t>(size);

    // 4バイトアライン
    ss.write_offset = (ss.write_offset + 3) & ~3u;

    return true;
}

size_t flash_fs_read(FlashFsFileId id, uint32_t offset, void *buf, size_t size) {
    if (id >= FLASH_FS_FILE_COUNT || FILE_TABLE[id].mode != FLASH_FS_MODE_STREAM) {
        return 0;
    }

    const auto &entry = FILE_TABLE[id];
    uint32_t total_data_size = entry.page_count * (PAGE_SIZE - PAGE_HEADER_SIZE);
    if (offset >= total_data_size) {
        return 0;
    }

    size_t remaining = size;
    auto *dst = static_cast<uint8_t *>(buf);

    // 最古ページから順に読む (ring buffer の論理順序)
    // 簡易実装: 物理ページ順に読む (完全なring対応は将来拡張)
    uint32_t byte_pos = 0;
    for (uint8_t p = 0; p < entry.page_count && remaining > 0; ++p) {
        uint32_t page_addr = get_page_address(id, p);
        PageHeader hdr;
        read_page_header(page_addr, &hdr);

        if (hdr.sequence == ERASED_WORD) {
            continue;
        }

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

        flash_fs_arch_read(page_addr + PAGE_HEADER_SIZE + skip, dst, to_read);
        dst += to_read;
        remaining -= to_read;
        byte_pos += data_len;
    }

    return size - remaining;
}

/* --- Block --- */

bool flash_fs_block_write(FlashFsFileId id, uint32_t offset, const void *data, size_t size) {
    if (id >= FLASH_FS_FILE_COUNT || FILE_TABLE[id].mode != FLASH_FS_MODE_BLOCK) {
        return false;
    }

    const auto &entry = FILE_TABLE[id];
    uint32_t capacity = entry.page_count * PAGE_SIZE;
    if (offset + size > capacity) {
        return false;
    }

    uint32_t addr = get_file_base_address(id) + offset;
    flash_fs_arch_write(addr, data, size);
    return true;
}

size_t flash_fs_block_read(FlashFsFileId id, uint32_t offset, void *buf, size_t size) {
    if (id >= FLASH_FS_FILE_COUNT || FILE_TABLE[id].mode != FLASH_FS_MODE_BLOCK) {
        return 0;
    }

    const auto &entry = FILE_TABLE[id];
    uint32_t capacity = entry.page_count * PAGE_SIZE;
    if (offset >= capacity) {
        return 0;
    }

    size_t to_read = size;
    if (offset + to_read > capacity) {
        to_read = capacity - offset;
    }

    flash_fs_arch_read(get_file_base_address(id) + offset, buf, to_read);
    return to_read;
}

/* --- 共通 --- */

void flash_fs_erase(FlashFsFileId id) {
    if (id >= FLASH_FS_FILE_COUNT) {
        return;
    }

    const auto &entry = FILE_TABLE[id];
    for (uint8_t p = 0; p < entry.page_count; ++p) {
        flash_fs_arch_page_erase(get_page_address(id, p));
    }

    // Stream状態をリセット
    if (entry.mode == FLASH_FS_MODE_STREAM) {
        auto &ss = s_state.stream[id];
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

bool flash_fs_get_info(FlashFsFileId id, FlashFsFileInfo *info) {
    if (id >= FLASH_FS_FILE_COUNT || info == nullptr) {
        return false;
    }

    const auto &entry = FILE_TABLE[id];
    info->id = entry.id;
    info->mode = entry.mode;
    info->name = entry.name;
    info->capacity = entry.page_count * PAGE_SIZE;

    if (entry.mode == FLASH_FS_MODE_STREAM) {
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

    return true;
}
