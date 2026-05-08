#include <catch2/catch_test_macros.hpp>
#include <osal/task>
#include <atomic>

static std::atomic<int> s_counter{0};

static void increment_task(void *param) {
    auto *val = reinterpret_cast<int *>(param);
    s_counter.fetch_add(*val);
}

TEST_CASE("task create/start/join", "[task]") {
    int arg = 42;
    s_counter.store(0);

    osal::task::config cfg;
    cfg.name = "test";
    cfg.priority = 10;
    cfg.stack_size = 4096;
    cfg.param = &arg;

    osal::task t;
    REQUIRE(t.create(increment_task, cfg));
    REQUIRE(t.start());
    t.join();
    REQUIRE(s_counter.load() == 42);
}

TEST_CASE("task constructor shorthand", "[task]") {
    int arg = 7;
    s_counter.store(0);

    osal::task::config cfg;
    cfg.name = "test2";
    cfg.param = &arg;
    cfg.stack_size = 4096;

    { osal::task t(increment_task, cfg); }  // destructor calls join
    REQUIRE(s_counter.load() == 7);
}

TEST_CASE("task sleep_for", "[task]") {
    osal::task::sleep_for(1);
}

TEST_CASE("task yield", "[task]") {
    osal::task::yield();
}
