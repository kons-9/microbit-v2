#include <catch2/catch_test_macros.hpp>

#include <option.h>
#include <result.h>

#include <string>

TEST_CASE("Option stores and exposes a value", "[library][option]") {
    auto value = library::Option<int>::some(42);

    REQUIRE(value.has_value());
    REQUIRE(*value == 42);
    REQUIRE(value.value_or(0) == 42);
}

TEST_CASE("Option represents no value", "[library][option]") {
    const auto value = library::Option<int>::none();

    REQUIRE_FALSE(value.has_value());
    REQUIRE(value.value_or(42) == 42);
}

TEST_CASE("Result stores either an ok value or an error", "[library][result]") {
    const auto success = library::Result<int, std::string>::ok(42);
    const auto failure = library::Result<int, std::string>::err("failed");

    REQUIRE(success.has_value());
    REQUIRE(success.value() == 42);
    REQUIRE(failure.has_error());
    REQUIRE(failure.error() == "failed");
}

TEST_CASE("Result supports success without a value", "[library][result]") {
    const auto success = library::Result<void, int>::ok();
    const auto failure = library::Result<void, int>::err(7);

    REQUIRE(success.has_value());
    REQUIRE_NOTHROW(success.value());
    REQUIRE(failure.has_error());
    REQUIRE(failure.error() == 7);
}
