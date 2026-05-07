#include <catch2/catch_test_macros.hpp>
#include <osal/timer>
#include <osal/task>
#include <atomic>

static std::atomic<int> g_cyclic_count{0};

static void cyclic_handler(void *) {
    g_cyclic_count.fetch_add(1);
}

TEST_CASE("cyclic_timer: fires periodically", "[timer]") {
    g_cyclic_count.store(0);
    osal::cyclic_timer timer(cyclic_handler, 20);
    timer.start();
    osal::task::sleep_for(150);
    timer.stop();
    int count = g_cyclic_count.load();
    // With 20ms interval over 150ms, expect roughly 5-8 fires
    REQUIRE(count >= 3);
    REQUIRE(count <= 15);
}

TEST_CASE("cyclic_timer: stop prevents further fires", "[timer]") {
    g_cyclic_count.store(0);
    osal::cyclic_timer timer(cyclic_handler, 20);
    timer.start();
    osal::task::sleep_for(60);
    timer.stop();
    int count_at_stop = g_cyclic_count.load();
    osal::task::sleep_for(80);
    int count_after = g_cyclic_count.load();
    // Should not have fired more after stop (allow 1 in-flight)
    REQUIRE(count_after <= count_at_stop + 1);
}

static std::atomic<int> g_oneshot_fired{0};

static void oneshot_handler(void *) {
    g_oneshot_fired.store(1);
}

TEST_CASE("oneshot_timer: fires once", "[timer]") {
    g_oneshot_fired.store(0);
    osal::oneshot_timer timer(oneshot_handler);
    timer.start(30);
    osal::task::sleep_for(10);
    REQUIRE(g_oneshot_fired.load() == 0);  // not yet
    osal::task::sleep_for(50);
    REQUIRE(g_oneshot_fired.load() == 1);  // fired
}

TEST_CASE("oneshot_timer: stop cancels", "[timer]") {
    g_oneshot_fired.store(0);
    osal::oneshot_timer timer(oneshot_handler);
    timer.start(50);
    osal::task::sleep_for(10);
    timer.stop();
    osal::task::sleep_for(80);
    REQUIRE(g_oneshot_fired.load() == 0);  // should not have fired
}
