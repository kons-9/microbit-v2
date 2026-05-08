#include <catch2/catch_test_macros.hpp>
#include "flash_fs.h"

#include <cstring>

/* テスト用: 仮想Flashリセット (linux バックエンドで定義) */
extern "C" void flash_fs_arch_test_reset();

struct FlashFsFixture {
    FlashFsFixture() {
        flash_fs_arch_test_reset();
        flash_fs_init();
    }
};

/* ================================================================== */
/*  Name resolution                                                   */
/* ================================================================== */

TEST_CASE_METHOD(FlashFsFixture, "flash_fs_get_name returns correct names", "[flash_fs]") {
    REQUIRE(std::strcmp(flash_fs_get_name(FLASH_FS_FILE_LOG), "log") == 0);
    REQUIRE(std::strcmp(flash_fs_get_name(FLASH_FS_FILE_SETTINGS), "settings") == 0);
    REQUIRE(std::strcmp(flash_fs_get_name(FLASH_FS_FILE_CALIB), "calib") == 0);
    REQUIRE(flash_fs_get_name(FLASH_FS_FILE_COUNT) == nullptr);
}

TEST_CASE_METHOD(FlashFsFixture, "flash_fs_find_by_name resolves names", "[flash_fs]") {
    REQUIRE(flash_fs_find_by_name("log") == FLASH_FS_FILE_LOG);
    REQUIRE(flash_fs_find_by_name("settings") == FLASH_FS_FILE_SETTINGS);
    REQUIRE(flash_fs_find_by_name("calib") == FLASH_FS_FILE_CALIB);
    REQUIRE(flash_fs_find_by_name("nonexist") == FLASH_FS_FILE_COUNT);
}

/* ================================================================== */
/*  Stream mode (Log)                                                 */
/* ================================================================== */

TEST_CASE_METHOD(FlashFsFixture, "Stream append and read back", "[flash_fs][stream]") {
    const uint8_t data[] = {0xDE, 0xAD, 0xBE, 0xEF};
    REQUIRE(flash_fs_append(FLASH_FS_FILE_LOG, data, sizeof(data)));

    uint8_t buf[4] = {};
    size_t n = flash_fs_read(FLASH_FS_FILE_LOG, 0, buf, sizeof(buf));
    REQUIRE(n == 4);
    REQUIRE(std::memcmp(buf, data, 4) == 0);
}

TEST_CASE_METHOD(FlashFsFixture, "Stream multiple appends", "[flash_fs][stream]") {
    uint32_t val1 = 0x11223344;
    uint32_t val2 = 0x55667788;
    REQUIRE(flash_fs_append(FLASH_FS_FILE_LOG, &val1, sizeof(val1)));
    REQUIRE(flash_fs_append(FLASH_FS_FILE_LOG, &val2, sizeof(val2)));

    uint32_t out = 0;
    REQUIRE(flash_fs_read(FLASH_FS_FILE_LOG, 0, &out, sizeof(out)) == 4);
    REQUIRE(out == 0x11223344);

    REQUIRE(flash_fs_read(FLASH_FS_FILE_LOG, 4, &out, sizeof(out)) == 4);
    REQUIRE(out == 0x55667788);
}

TEST_CASE_METHOD(FlashFsFixture, "Stream erase clears data", "[flash_fs][stream]") {
    uint32_t val = 0xAAAAAAAA;
    flash_fs_append(FLASH_FS_FILE_LOG, &val, sizeof(val));

    flash_fs_erase(FLASH_FS_FILE_LOG);

    FlashFsFileInfo info;
    flash_fs_get_info(FLASH_FS_FILE_LOG, &info);
    REQUIRE(info.used == 0);
}

TEST_CASE_METHOD(FlashFsFixture, "Stream wraps to next page", "[flash_fs][stream]") {
    // ページサイズ 4096 - ヘッダ 8 = 4088 bytes usable per page
    uint8_t chunk[1024];
    std::memset(chunk, 0xAB, sizeof(chunk));

    // 4回書くと4088近くになり、5回目で次ページへ
    for (int i = 0; i < 5; ++i) {
        REQUIRE(flash_fs_append(FLASH_FS_FILE_LOG, chunk, sizeof(chunk)));
    }

    // 読み返し (最初のページの最初のチャンク)
    uint8_t read_buf[1024];
    size_t n = flash_fs_read(FLASH_FS_FILE_LOG, 0, read_buf, sizeof(read_buf));
    REQUIRE(n == 1024);
    REQUIRE(read_buf[0] == 0xAB);
    REQUIRE(read_buf[1023] == 0xAB);
}

TEST_CASE_METHOD(FlashFsFixture, "Stream rejects oversized append", "[flash_fs][stream]") {
    // ページサイズ - ヘッダ = 4092。それを超えるデータは拒否
    uint8_t big[4093];
    REQUIRE_FALSE(flash_fs_append(FLASH_FS_FILE_LOG, big, sizeof(big)));
}

TEST_CASE_METHOD(FlashFsFixture, "Stream rejects wrong file mode", "[flash_fs][stream]") {
    uint8_t data = 0;
    REQUIRE_FALSE(flash_fs_append(FLASH_FS_FILE_SETTINGS, &data, 1));
}

/* ================================================================== */
/*  Block mode (Settings)                                             */
/* ================================================================== */

TEST_CASE_METHOD(FlashFsFixture, "Block write and read", "[flash_fs][block]") {
    uint32_t val = 0xCAFEBABE;
    REQUIRE(flash_fs_block_write(FLASH_FS_FILE_SETTINGS, 0, &val, sizeof(val)));

    uint32_t out = 0;
    REQUIRE(flash_fs_block_read(FLASH_FS_FILE_SETTINGS, 0, &out, sizeof(out)) == 4);
    REQUIRE(out == 0xCAFEBABE);
}

TEST_CASE_METHOD(FlashFsFixture, "Block write at offset", "[flash_fs][block]") {
    uint32_t a = 0x11111111;
    uint32_t b = 0x22222222;
    REQUIRE(flash_fs_block_write(FLASH_FS_FILE_SETTINGS, 0, &a, sizeof(a)));
    REQUIRE(flash_fs_block_write(FLASH_FS_FILE_SETTINGS, 100, &b, sizeof(b)));

    uint32_t out = 0;
    REQUIRE(flash_fs_block_read(FLASH_FS_FILE_SETTINGS, 100, &out, sizeof(out)) == 4);
    REQUIRE(out == 0x22222222);
}

TEST_CASE_METHOD(FlashFsFixture, "Block rejects out-of-bounds write", "[flash_fs][block]") {
    uint8_t data = 0;
    // 1 page = 4096 bytes, writing at 4096 is out of bounds
    REQUIRE_FALSE(flash_fs_block_write(FLASH_FS_FILE_SETTINGS, 4096, &data, 1));
}

TEST_CASE_METHOD(FlashFsFixture, "Block rejects wrong file mode", "[flash_fs][block]") {
    uint8_t data = 0;
    REQUIRE_FALSE(flash_fs_block_write(FLASH_FS_FILE_LOG, 0, &data, 1));
}

/* ================================================================== */
/*  Info                                                              */
/* ================================================================== */

TEST_CASE_METHOD(FlashFsFixture, "flash_fs_get_info returns correct capacity", "[flash_fs]") {
    FlashFsFileInfo info;
    REQUIRE(flash_fs_get_info(FLASH_FS_FILE_LOG, &info));
    REQUIRE(info.capacity == 4 * 4096);
    REQUIRE(info.mode == FLASH_FS_MODE_STREAM);
    REQUIRE(std::strcmp(info.name, "log") == 0);

    REQUIRE(flash_fs_get_info(FLASH_FS_FILE_SETTINGS, &info));
    REQUIRE(info.capacity == 4096);
    REQUIRE(info.mode == FLASH_FS_MODE_BLOCK);
}
