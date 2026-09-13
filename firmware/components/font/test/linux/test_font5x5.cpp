#include <catch2/catch_test_macros.hpp>

#include <font5x5.h>

#include <cstdint>

static_assert(font5x5::CHAR_0[0] == 0x0E);
static_assert(font5x5::CHAR_A[2] == 0x1F);
static_assert(font5x5::SYM_FULL[4] == 0x1F);

TEST_CASE("font5x5 renders zero as a 5x5 bitmap", "[font5x5]") {
    constexpr uint8_t expected_zero[5] = {
        0b01110,
        0b10001,
        0b10001,
        0b10001,
        0b01110,
    };

    const auto zero = font5x5::get('0');
    REQUIRE(zero.has_value());

    for (int row = 0; row < 5; ++row) {
        REQUIRE((*zero)[row] == expected_zero[row]);
    }
}

TEST_CASE("font5x5 maps upper and lower case letters identically", "[font5x5]") {
    const auto upper = font5x5::get('A');
    const auto lower = font5x5::get('a');

    REQUIRE(upper.has_value());
    REQUIRE(lower.has_value());
    REQUIRE(*lower == *upper);
    REQUIRE((*upper)[0] == 0x0E);
    REQUIRE((*upper)[2] == 0x1F);
}

TEST_CASE("font5x5 rejects unsupported characters", "[font5x5]") {
    REQUIRE_FALSE(font5x5::get(' ').has_value());
    REQUIRE_FALSE(font5x5::get('!').has_value());
    REQUIRE_FALSE(font5x5::get(static_cast<char>(0x7F)).has_value());
}

TEST_CASE("font5x5 exposes symbol glyphs", "[font5x5]") {
    REQUIRE(font5x5::SYM_CHECK[0] == 0x00);
    REQUIRE(font5x5::SYM_CROSS[2] == 0x04);
    REQUIRE(font5x5::SYM_EMPTY[0] == 0x00);
    REQUIRE(font5x5::SYM_FULL[0] == 0x1F);
}
