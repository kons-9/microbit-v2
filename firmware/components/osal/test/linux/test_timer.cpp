#include <catch2/catch_test_macros.hpp>
#include <osal/timer>
#include <osal/task>
#include <atomic>

static std::atomic<int> s_cyclicCount{0};
static std::atomic<int> s_oneshotCount{0};

static void cyclic_handler(void * /*param*/) {
    s_cyclicCount.fetch_add(1);
}

static void oneshot_handler(void * /*param*/) {
    s_oneshotCount.fetch_add(1);
}

TEST_CASE("cyclic_timer fires repeatedly", "[timer]") {
    s_cyclicCount.store(0);
    {
        osal::cyclic_timer timer(cyclic_handler, 10);
        timer.start();
        osal::task::sleep_for(55);
        timer.stop();
    }
    int count = s_cyclicCount.load();
    REQUIRE(count >= 3);
}

TEST_CASE("oneshot_timer fires once", "[timer]") {
    s_oneshotCount.store(0);
    {
        osal::oneshot_timer timer(oneshot_handler);
        timer.start(10);
        osal::task::sleep_for(50);
    }
    REQUIRE(s_oneshotCount.load() == 1);
}
