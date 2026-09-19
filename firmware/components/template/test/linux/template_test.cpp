#include <catch2/catch_test_macros.hpp>
#include "template.h"

TEST_CASE("template_add basic", "[template]") {
    REQUIRE(tmpl::add(1, 2) == 3);
    REQUIRE(tmpl::add(-5, 5) == 0);
    REQUIRE(tmpl::add(0, 0) == 0);
}
