#include <catch2/catch_test_macros.hpp>
#include "flash_log.hpp"
#include "flash_fs.h"

#include <cstring>

/* Flash テスト用リセット */
extern "C" void flash_fs_arch_test_reset();

/* get_uptime_ms16 モック */
static uint16_t s_mockUptime = 0;
namespace flash_log {
uint16_t get_uptime_ms16() {
    return s_mockUptime;
}
}  // namespace flash_log

struct FlashLogFixture {
    FlashLogFixture() {
        flash_fs_arch_test_reset();
        flash_fs::init();
        s_mockUptime = 0;
    }
};

/* ================================================================== */
/*  Write / Read back                                                 */
/* ================================================================== */

TEST_CASE_METHOD(FlashLogFixture, "Write EventEntry and read back raw", "[flash_log]") {
    flash_log::EventEntry ev{};
    ev.event_id = 42;
    ev.param = 0xDEADBEEF;
    s_mockUptime = 1234;

    REQUIRE(flash_log::write(ev));

    // Read raw bytes from flash
    uint8_t buf[64];
    size_t n = flash_fs::read(flash_fs::FILE_LOG, 0, buf, sizeof(buf));
    REQUIRE(n >= sizeof(flash_log::RecordHeader) + sizeof(flash_log::EventEntry));

    // Parse header
    flash_log::RecordHeader hdr;
    std::memcpy(&hdr, buf, sizeof(hdr));
    REQUIRE(hdr.version == flash_log::RECORD_VERSION);
    REQUIRE(hdr.type == static_cast<uint8_t>(flash_log::Type::Event));
    REQUIRE(hdr.size == sizeof(flash_log::EventEntry));
    REQUIRE(hdr.timestamp_ms == 1234);

    // Parse payload
    flash_log::EventEntry read_ev;
    std::memcpy(&read_ev, buf + sizeof(hdr), sizeof(read_ev));
    REQUIRE(read_ev.event_id == 42);
    REQUIRE(read_ev.param == 0xDEADBEEF);
}

TEST_CASE_METHOD(FlashLogFixture, "Write CrashEntry", "[flash_log]") {
    flash_log::CrashEntry crash{};
    crash.fault_type = 3;
    crash.pc = 0x08001234;
    crash.lr = 0x08005678;
    crash.sp = 0x20004000;
    s_mockUptime = 5000;

    REQUIRE(flash_log::write(crash));

    uint8_t buf[128];
    size_t n = flash_fs::read(flash_fs::FILE_LOG, 0, buf, sizeof(buf));
    REQUIRE(n >= sizeof(flash_log::RecordHeader) + sizeof(flash_log::CrashEntry));

    flash_log::RecordHeader hdr;
    std::memcpy(&hdr, buf, sizeof(hdr));
    REQUIRE(hdr.type == static_cast<uint8_t>(flash_log::Type::Crash));
    REQUIRE(hdr.size == sizeof(flash_log::CrashEntry));

    flash_log::CrashEntry read_crash;
    std::memcpy(&read_crash, buf + sizeof(hdr), sizeof(read_crash));
    REQUIRE(read_crash.fault_type == 3);
    REQUIRE(read_crash.pc == 0x08001234);
}

TEST_CASE_METHOD(FlashLogFixture, "Write BleEntry", "[flash_log]") {
    flash_log::BleEntry ble{};
    ble.addr[0] = 0xAA;
    ble.addr[5] = 0xBB;
    ble.rssi = -72;
    ble.event_type = 1;

    REQUIRE(flash_log::write(ble));

    uint8_t buf[32];
    size_t n = flash_fs::read(flash_fs::FILE_LOG, 0, buf, sizeof(buf));
    REQUIRE(n >= sizeof(flash_log::RecordHeader) + sizeof(flash_log::BleEntry));

    flash_log::RecordHeader hdr;
    std::memcpy(&hdr, buf, sizeof(hdr));
    REQUIRE(hdr.type == static_cast<uint8_t>(flash_log::Type::Ble));

    flash_log::BleEntry read_ble;
    std::memcpy(&read_ble, buf + sizeof(hdr), sizeof(read_ble));
    REQUIRE(read_ble.addr[0] == 0xAA);
    REQUIRE(read_ble.addr[5] == 0xBB);
    REQUIRE(read_ble.rssi == -72);
}

TEST_CASE_METHOD(FlashLogFixture, "Multiple writes accumulate", "[flash_log]") {
    flash_log::EventEntry ev1{};
    ev1.event_id = 1;
    ev1.param = 100;
    s_mockUptime = 10;
    REQUIRE(flash_log::write(ev1));

    flash_log::EventEntry ev2{};
    ev2.event_id = 2;
    ev2.param = 200;
    s_mockUptime = 20;
    REQUIRE(flash_log::write(ev2));

    // Second record offset = sizeof(RecordHeader) + sizeof(EventEntry) aligned to 4
    constexpr size_t record_size = sizeof(flash_log::RecordHeader) + sizeof(flash_log::EventEntry);
    constexpr size_t aligned_size = (record_size + 3) & ~3u;

    uint8_t buf[64];
    size_t n = flash_fs::read(flash_fs::FILE_LOG, aligned_size, buf, sizeof(buf));
    REQUIRE(n >= sizeof(flash_log::RecordHeader) + sizeof(flash_log::EventEntry));

    flash_log::RecordHeader hdr2;
    std::memcpy(&hdr2, buf, sizeof(hdr2));
    REQUIRE(hdr2.timestamp_ms == 20);

    flash_log::EventEntry read_ev2;
    std::memcpy(&read_ev2, buf + sizeof(hdr2), sizeof(read_ev2));
    REQUIRE(read_ev2.event_id == 2);
    REQUIRE(read_ev2.param == 200);
}
