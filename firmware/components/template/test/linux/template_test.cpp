#include <catch2/catch_test_macros.hpp>
#include "template.h"

TEST_CASE("template_add basic", "[template]") {
    REQUIRE(template_add(1, 2) == 3);
    REQUIRE(template_add(-5, 5) == 0);
    REQUIRE(template_add(0, 0) == 0);
}
