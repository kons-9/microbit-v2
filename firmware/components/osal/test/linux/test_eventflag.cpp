#include <catch2/catch_test_macros.hpp>
#include <osal/eventflag>
#include <osal/task>
#include <atomic>

TEST_CASE("eventflag: set and get", "[eventflag]") {
    osal::event_flag ef(0);
    REQUIRE(ef.get() == 0);
    ef.set(0x01);
    REQUIRE(ef.get() == 0x01);
    ef.set(0x04);
    REQUIRE(ef.get() == 0x05);
}

TEST_CASE("eventflag: clear", "[eventflag]") {
    osal::event_flag ef(0xFF);
    ef.clear(0x0F);
    REQUIRE(ef.get() == 0xF0);
}

TEST_CASE("eventflag: wait any", "[eventflag]") {
    osal::event_flag ef(0x05);
    uint32_t result = ef.wait(0x01, osal::event_flag::any, 100);
    REQUIRE(result == 0x01);
}

TEST_CASE("eventflag: wait all", "[eventflag]") {
    osal::event_flag ef(0x07);
    uint32_t result = ef.wait(0x05, osal::event_flag::all, 100);
    REQUIRE(result == 0x05);
}

TEST_CASE("eventflag: wait timeout", "[eventflag]") {
    osal::event_flag ef(0);
    uint32_t result = ef.wait(0x01, osal::event_flag::any, 50);
    REQUIRE(result == 0);
}

static osal::event_flag *g_ef_ptr = nullptr;

static void setter_entry(void *) {
    osal::task::sleep_for(30);
    g_ef_ptr->set(0x10);
}

TEST_CASE("eventflag: cross-thread set/wait", "[eventflag]") {
    osal::event_flag ef(0);
    g_ef_ptr = &ef;

    osal::task::config cfg;
    cfg.stack_size = 4096;
    osal::task t(setter_entry, cfg);

    uint32_t result = ef.wait(0x10, osal::event_flag::any, 1000);
    REQUIRE(result == 0x10);
    t.join();
}
