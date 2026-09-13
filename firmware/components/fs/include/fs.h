#pragma once

/**
 * @file fs.h
 * @brief Flash ファイルシステム抽象化
 *
 * enum class ベースの固定ファイルIDで管理する最小ファイルシステム。
 * Flash ドライバはアプリケーションからコンストラクタ注入する。
 */

#include <cstddef>
#include <cstdint>

#include <flash.h>
#include <io_stream.h>
#include <option.h>
#include <result.h>

namespace fs {

enum class Error : uint8_t {
    InvalidFile,
    InvalidArgument,
    WrongFileType,
    OutOfBounds,
    TooLarge,
};

enum class FileId : uint8_t {
    Log = 0,
    Settings = 1,
    Calib = 2,
    Count = 3,
};

enum class FileType : uint8_t {
    RingBuffer = 0,
    Fixed = 1,
};

struct FileInfo {
    FileId id;
    FileType type;
    const char *name;
    uint32_t capacity;
    uint32_t used;
};

/**
 * @brief Flash ファイルシステム
 *
 * インスタンスごとに Flash とランタイム状態を保持する。これにより、
 * グローバルな Flash や FileSystem に依存せず、アプリケーションの
 * Config から注入できる。
 */
class FileSystem {
  public:
    explicit FileSystem(drivers::Flash &flash);
    ~FileSystem() = default;

    int32_t init();
    library::Option<const char *> get_name(FileId id);
    library::Option<FileId> find_by_name(const char *name);
    library::Result<void, Error> append(FileId id, const void *data, size_t size);
    size_t read(FileId id, uint32_t offset, void *buf, size_t size);
    library::Result<void, Error> block_write(FileId id, uint32_t offset, const void *data, size_t size);
    size_t block_read(FileId id, uint32_t offset, void *buf, size_t size);
    void erase(FileId id);
    library::Result<void, Error> get_info(FileId id, FileInfo *info);

  private:
    struct StreamState {
        uint32_t current_page;
        uint32_t write_offset;
        uint32_t sequence;
    };

    uint32_t get_file_base_address(FileId id) const;
    uint32_t get_page_address(FileId id, uint32_t page_index) const;
    void read_page_header(uint32_t page_addr, void *header);
    void write_page_header(uint32_t page_addr, const void *header);
    uint32_t scan_write_offset(uint32_t page_addr);
    void init_stream_state(FileId id);
    void advance_to_next_page(FileId id);

    drivers::Flash &flash_;
    uint32_t base_address_[static_cast<size_t>(FileId::Count)] = {};
    StreamState stream_[static_cast<size_t>(FileId::Count)] = {};
    bool initialized_ = false;
};

/**
 * @brief Ring-buffer file exposed as an io::Stream.
 *
 * write() appends one record to the file. read() reads sequentially from the
 * oldest available byte. FileSystem remains responsible for the on-flash
 * layout and ring-buffer rotation.
 */
class RingBufferFile final : public io::Stream {
  public:
    RingBufferFile(FileSystem &file_system, FileId id);

    int32_t write(const uint8_t *data, size_t len) override;
    int32_t read(uint8_t *buf, size_t buf_len, uint32_t timeout_ms) override;

    void rewind();

  private:
    FileSystem &file_system_;
    FileId id_;
    uint32_t read_offset_ = 0;
};

/**
 * @brief Fixed-size file exposed as an io::Stream.
 *
 * The read and write cursors are independent. This is useful for settings or
 * calibration data when sequential access is sufficient.
 */
class FixedFile final : public io::Stream {
  public:
    FixedFile(FileSystem &file_system, FileId id);

    int32_t write(const uint8_t *data, size_t len) override;
    int32_t read(uint8_t *buf, size_t buf_len, uint32_t timeout_ms) override;

    void rewind_read();
    void rewind_write();

  private:
    FileSystem &file_system_;
    FileId id_;
    uint32_t read_offset_ = 0;
    uint32_t write_offset_ = 0;
};

}  // namespace fs
