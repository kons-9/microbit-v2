/**
 * @file file_commands.cpp
 * @brief ファイルシステム関連Shellコマンド
 */

#include "shell.h"
#include "io_stream.h"

#include <cstring>

namespace shell {

void Shell::cmd_ls() {
    printf("%-10s %6s/%6s  %s\r\n", "NAME", "USED", "CAP", "TYPE");
    for (size_t i = 0; i < static_cast<size_t>(fs::FileId::Count); ++i) {
        fs::FileInfo info;
        if (file_system_ != nullptr && file_system_->get_info(static_cast<fs::FileId>(i), &info)) {
            const char *type_str = (info.type == fs::FileType::RingBuffer) ? "ring" : "fixed";
            printf("%-10s %6lu/%6lu  [%s]\r\n",
                   info.name,
                   static_cast<unsigned long>(info.used),
                   static_cast<unsigned long>(info.capacity),
                   type_str);
        }
    }
}

void Shell::cmd_cat(int32_t argc, const char *const *argv) {
    if (argc < 2) {
        puts("Usage: cat <file> [--hex]\r\n");
        return;
    }

    if (file_system_ == nullptr) {
        puts("File system is not initialized\r\n");
        return;
    }

    auto id = file_system_->find_by_name(argv[1]);
    if (!id) {
        printf("Unknown file: %s\r\n", argv[1]);
        return;
    }

    bool hex_mode = false;
    if (argc >= 3 && std::strcmp(argv[2], "--hex") == 0) {
        hex_mode = true;
    }

    fs::FileInfo info;
    file_system_->get_info(*id, &info);

    uint32_t offset = 0;
    uint32_t total = info.used;

    while (offset < total) {
        size_t chunk = sizeof(file_buf_);
        if (offset + chunk > total) {
            chunk = total - offset;
        }

        size_t read_len = 0;
        if (info.type == fs::FileType::RingBuffer) {
            read_len = file_system_->read(*id, offset, file_buf_, chunk);
        } else {
            read_len = file_system_->block_read(*id, offset, file_buf_, chunk);
        }

        if (read_len == 0) {
            break;
        }

        if (hex_mode) {
            for (size_t i = 0; i < read_len; ++i) {
                if (i % 16 == 0) {
                    printf("%08lX: ", static_cast<unsigned long>(offset + i));
                }
                printf("%02X ", file_buf_[i]);
                if (i % 16 == 15 || i == read_len - 1) {
                    puts("\r\n");
                }
            }
        } else if (stream_ != nullptr) {
            if (*id == fs::FileId::LogRing) {
                /* Log records are aligned to 4-byte flash words.  Older
                 * records therefore contain 0xFF padding between lines.
                 * Hide that padding in text mode; --hex still exposes it. */
                size_t text_len = 0;
                for (size_t i = 0; i < read_len; ++i) {
                    if (file_buf_[i] != 0xFF) {
                        file_buf_[text_len++] = file_buf_[i];
                    }
                }
                if (text_len > 0) {
                    stream_->write(file_buf_, text_len);
                }
            } else {
                stream_->write(file_buf_, read_len);
            }
        }

        offset += static_cast<uint32_t>(read_len);
    }
    puts("\r\n");
}

void Shell::cmd_erase(int32_t argc, const char *const *argv) {
    if (argc < 2) {
        puts("Usage: erase <file>\r\n");
        return;
    }

    if (file_system_ == nullptr) {
        puts("File system is not initialized\r\n");
        return;
    }

    auto id = file_system_->find_by_name(argv[1]);
    if (!id) {
        printf("Unknown file: %s\r\n", argv[1]);
        return;
    }

    file_system_->erase(*id);
    puts("OK\r\n");
}

}  // namespace shell
