#include <catch2/catch_test_macros.hpp>
#include "flash.h"
#include "fs.h"

#include <cstring>

/* テスト用: 仮想Flashリセット (linux バックエンドで定義) */
extern "C" void flash_test_reset();

struct FsFixture {
    drivers::Flash flash;
    fs::FileSystem file_system;

    FsFixture()
        : file_system(flash) {
        flash_test_reset();
        file_system.init();
    }
};

/* ================================================================== */
/*  Name resolution                                                   */
/* ================================================================== */

TEST_CASE_METHOD(FsFixture, "fs_get_name returns correct names", "[fs]") {
    REQUIRE(std::strcmp(*file_system.get_name(fs::FileId::Log), "log") == 0);
    REQUIRE(std::strcmp(*file_system.get_name(fs::FileId::Settings), "settings") == 0);
    REQUIRE(std::strcmp(*file_system.get_name(fs::FileId::Calib), "calib") == 0);
    REQUIRE_FALSE(file_system.get_name(fs::FileId::Count).has_value());
}

TEST_CASE_METHOD(FsFixture, "fs_find_by_name resolves names", "[fs]") {
    REQUIRE(*file_system.find_by_name("log") == fs::FileId::Log);
    REQUIRE(*file_system.find_by_name("settings") == fs::FileId::Settings);
    REQUIRE(*file_system.find_by_name("calib") == fs::FileId::Calib);
    REQUIRE_FALSE(file_system.find_by_name("nonexist").has_value());
}

/* ================================================================== */
/*  RingBuffer file (Log)                                             */
/* ================================================================== */

TEST_CASE_METHOD(FsFixture, "Stream append and read back", "[fs][stream]") {
    const uint8_t data[] = {0xDE, 0xAD, 0xBE, 0xEF};
    REQUIRE(file_system.append(fs::FileId::Log, data, sizeof(data)));

    uint8_t buf[4] = {};
    size_t n = file_system.read(fs::FileId::Log, 0, buf, sizeof(buf));
    REQUIRE(n == 4);
    REQUIRE(std::memcmp(buf, data, 4) == 0);
}

TEST_CASE_METHOD(FsFixture, "Stream multiple appends", "[fs][stream]") {
    uint32_t val1 = 0x11223344;
    uint32_t val2 = 0x55667788;
    REQUIRE(file_system.append(fs::FileId::Log, &val1, sizeof(val1)));
    REQUIRE(file_system.append(fs::FileId::Log, &val2, sizeof(val2)));

    uint32_t out = 0;
    REQUIRE(file_system.read(fs::FileId::Log, 0, &out, sizeof(out)) == 4);
    REQUIRE(out == 0x11223344);

    REQUIRE(file_system.read(fs::FileId::Log, 4, &out, sizeof(out)) == 4);
    REQUIRE(out == 0x55667788);
}

TEST_CASE_METHOD(FsFixture, "Stream erase clears data", "[fs][stream]") {
    uint32_t val = 0xAAAAAAAA;
    file_system.append(fs::FileId::Log, &val, sizeof(val));

    file_system.erase(fs::FileId::Log);

    fs::FileInfo info;
    file_system.get_info(fs::FileId::Log, &info);
    REQUIRE(info.used == 0);
}

TEST_CASE_METHOD(FsFixture, "Stream wraps to next page", "[fs][stream]") {
    // ページサイズ 4096 - ヘッダ 8 = 4088 bytes usable per page
    uint8_t chunk[1024];
    std::memset(chunk, 0xAB, sizeof(chunk));

    // 4回書くと4088近くになり、5回目で次ページへ
    for (int i = 0; i < 5; ++i) {
        REQUIRE(file_system.append(fs::FileId::Log, chunk, sizeof(chunk)));
    }

    // 読み返し (最初のページの最初のチャンク)
    uint8_t read_buf[1024];
    size_t n = file_system.read(fs::FileId::Log, 0, read_buf, sizeof(read_buf));
    REQUIRE(n == 1024);
    REQUIRE(read_buf[0] == 0xAB);
    REQUIRE(read_buf[1023] == 0xAB);
}

TEST_CASE_METHOD(FsFixture, "Ring buffer reads pages in sequence order", "[fs][stream]") {
    uint8_t chunk[4088] = {};

    for (uint8_t marker = 1; marker <= 5; ++marker) {
        chunk[0] = marker;
        REQUIRE(file_system.append(fs::FileId::Log, chunk, sizeof(chunk)));
    }

    uint8_t oldest = 0;
    REQUIRE(file_system.read(fs::FileId::Log, 0, &oldest, sizeof(oldest)) == 1);
    REQUIRE(oldest == 2);
}

TEST_CASE_METHOD(FsFixture, "Stream rejects oversized append", "[fs][stream]") {
    // ページサイズ - ヘッダ = 4092。それを超えるデータは拒否
    uint8_t big[4093];
    REQUIRE_FALSE(file_system.append(fs::FileId::Log, big, sizeof(big)));
}

TEST_CASE_METHOD(FsFixture, "Ring buffer rejects wrong file type", "[fs][stream]") {
    uint8_t data = 0;
    REQUIRE_FALSE(file_system.append(fs::FileId::Settings, &data, 1));
}

TEST_CASE_METHOD(FsFixture, "RingBufferFile implements io::Stream", "[fs][stream]") {
    fs::RingBufferFile file(file_system, fs::FileId::Log);
    const uint8_t data[] = {0x01, 0x02, 0x03};
    REQUIRE(file.write(data, sizeof(data)) == static_cast<int32_t>(sizeof(data)));

    uint8_t out[sizeof(data)] = {};
    REQUIRE(file.read(out, sizeof(out), 0) == static_cast<int32_t>(sizeof(out)));
    REQUIRE(std::memcmp(out, data, sizeof(data)) == 0);
}

/* ================================================================== */
/*  Fixed file (Settings)                                             */
/* ================================================================== */

TEST_CASE_METHOD(FsFixture, "Block write and read", "[fs][block]") {
    uint32_t val = 0xCAFEBABE;
    REQUIRE(file_system.block_write(fs::FileId::Settings, 0, &val, sizeof(val)));

    uint32_t out = 0;
    REQUIRE(file_system.block_read(fs::FileId::Settings, 0, &out, sizeof(out)) == 4);
    REQUIRE(out == 0xCAFEBABE);
}

TEST_CASE_METHOD(FsFixture, "Block write at offset", "[fs][block]") {
    uint32_t a = 0x11111111;
    uint32_t b = 0x22222222;
    REQUIRE(file_system.block_write(fs::FileId::Settings, 0, &a, sizeof(a)));
    REQUIRE(file_system.block_write(fs::FileId::Settings, 100, &b, sizeof(b)));

    uint32_t out = 0;
    REQUIRE(file_system.block_read(fs::FileId::Settings, 100, &out, sizeof(out)) == 4);
    REQUIRE(out == 0x22222222);
}

TEST_CASE_METHOD(FsFixture, "Block rejects out-of-bounds write", "[fs][block]") {
    uint8_t data = 0;
    // 1 page = 4096 bytes, writing at 4096 is out of bounds
    REQUIRE_FALSE(file_system.block_write(fs::FileId::Settings, 4096, &data, 1));
}

TEST_CASE_METHOD(FsFixture, "Fixed file rejects wrong file type", "[fs][block]") {
    uint8_t data = 0;
    REQUIRE_FALSE(file_system.block_write(fs::FileId::Log, 0, &data, 1));
}

TEST_CASE_METHOD(FsFixture, "FixedFile implements io::Stream", "[fs][block]") {
    fs::FixedFile file(file_system, fs::FileId::Settings);
    const uint8_t data[] = {0xAA, 0xBB};
    REQUIRE(file.write(data, sizeof(data)) == static_cast<int32_t>(sizeof(data)));

    uint8_t out[sizeof(data)] = {};
    REQUIRE(file.read(out, sizeof(out), 0) == static_cast<int32_t>(sizeof(out)));
    REQUIRE(std::memcmp(out, data, sizeof(data)) == 0);
}

/* ================================================================== */
/*  Info                                                              */
/* ================================================================== */

TEST_CASE_METHOD(FsFixture, "fs_get_info returns correct capacity", "[fs]") {
    fs::FileInfo info;
    REQUIRE(file_system.get_info(fs::FileId::Log, &info));
    REQUIRE(info.capacity == 4 * 4096);
    REQUIRE(info.type == fs::FileType::RingBuffer);
    REQUIRE(std::strcmp(info.name, "log") == 0);

    REQUIRE(file_system.get_info(fs::FileId::Settings, &info));
    REQUIRE(info.capacity == 4096);
    REQUIRE(info.type == fs::FileType::Fixed);
}
