/**
 * @file test_fs.cpp
 * @brief Flash FS統合テスト
 */

#define LOG_TAG "ITEST"
#include "integration_test.h"

#include "log.h"

#include <cstring>

namespace integration_test {

void run_fs_test(Context &context) {
    LOG_I("=== Flash FS Test ===");

    auto init_result = context.config.file_system.init();
    LOG_I("fs_init: %ld", init_result);
    context.assert_true(init_result == 0, "fs_init");

    auto log_name = context.config.file_system.get_name(fs::FileId::LogRing);
    LOG_I("file[0] name: %s", log_name ? *log_name : "(null)");
    context.assert_true(log_name.has_value(), "fs::get_name(LOG)");

    fs::FileInfo file_info;
    auto info_result = context.config.file_system.get_info(fs::FileId::Settings, &file_info);
    LOG_I("settings: type=%lu, capacity=%lu, used=%lu",
          static_cast<uint32_t>(static_cast<uint8_t>(file_info.type)),
          file_info.capacity,
          file_info.used);
    context.assert_true(info_result.has_value(), "fs::get_info(SETTINGS)");

    const uint32_t test_val = 0xDEADBEEF;
    auto write_result = context.config.file_system.block_write(fs::FileId::Settings, 0, &test_val, sizeof(test_val));
    context.assert_true(write_result.has_value(), "fs_block_write");

    uint32_t read_val = 0;
    auto read_sz = context.config.file_system.block_read(fs::FileId::Settings, 0, &read_val, sizeof(read_val));
    LOG_I("read back: 0x%lx (size=%lu)", read_val, static_cast<uint32_t>(read_sz));
    context.assert_true(read_sz == sizeof(test_val) && read_val == test_val, "fs_block_read matches");

    context.config.file_system.erase(fs::FileId::LogRing);
    const char msg[] = "hello";
    auto append_result = context.config.file_system.append(fs::FileId::LogRing, msg, sizeof(msg));
    context.assert_true(append_result.has_value(), "fs_append");

    char read_buf[16] = {};
    auto stream_sz = context.config.file_system.read(fs::FileId::LogRing, 0, read_buf, sizeof(read_buf));
    LOG_I("stream read: \"%s\" (size=%lu)", read_buf, static_cast<uint32_t>(stream_sz));
    context.assert_true(stream_sz >= sizeof(msg) && std::memcmp(read_buf, msg, sizeof(msg)) == 0, "fs_read matches");
}

}  // namespace integration_test
