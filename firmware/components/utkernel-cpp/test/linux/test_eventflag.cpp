#include <catch2/catch_test_macros.hpp>
#include <utkernel/eventflag>

TEST_CASE("event_flag set/wait any", "[eventflag]") {
    utkernel::event_flag ef;
    ef.set(0x01);
    uint32_t result = ef.wait(0x01, utkernel::event_flag::wait_mode::Any, 100);
    REQUIRE(result == 0x01);
}

TEST_CASE("event_flag set/wait all", "[eventflag]") {
    utkernel::event_flag ef;
    ef.set(0x03);
    uint32_t result = ef.wait(0x03, utkernel::event_flag::wait_mode::All, 100);
    REQUIRE(result == 0x03);
}

TEST_CASE("event_flag wait timeout", "[eventflag]") {
    utkernel::event_flag ef;
    uint32_t result = ef.wait(0x01, utkernel::event_flag::wait_mode::Any, 10);
    REQUIRE(result == 0);
}

TEST_CASE("event_flag clear", "[eventflag]") {
    utkernel::event_flag ef;
    ef.set(0x03);
    ef.clear(0x01);
    REQUIRE(ef.get() == 0x02);
}

TEST_CASE("event_flag get", "[eventflag]") {
    utkernel::event_flag ef;
    REQUIRE(ef.get() == 0);
    ef.set(0xFF);
    REQUIRE(ef.get() == 0xFF);
}
