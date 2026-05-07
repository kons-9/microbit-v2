#include <catch2/catch_test_macros.hpp>
#include <osal/task>
#include <atomic>

static std::atomic<int> g_task_ran{0};

static void task_entry(void *param) {
    auto *val = static_cast<int *>(param);
    g_task_ran.store(*val);
}

TEST_CASE("task: create and start", "[task]") {
    g_task_ran.store(0);
    int param = 42;
    osal::task::config cfg;
    cfg.name = "test_task";
    cfg.priority = 10;
    cfg.stack_size = 4096;
    cfg.param = &param;

    osal::task t;
    REQUIRE(t.create(task_entry, cfg) == true);
    REQUIRE(t.start() == true);
    t.join();
    REQUIRE(g_task_ran.load() == 42);
}

TEST_CASE("task: constructor auto-starts", "[task]") {
    g_task_ran.store(0);
    int param = 99;
    osal::task::config cfg;
    cfg.param = &param;
    cfg.stack_size = 4096;

    {
        osal::task t(task_entry, cfg);
        // destructor joins
    }
    REQUIRE(g_task_ran.load() == 99);
}

static void sleep_entry(void *) {
    osal::task::sleep_for(50);
    g_task_ran.store(1);
}

TEST_CASE("task: sleep_for", "[task]") {
    g_task_ran.store(0);
    osal::task::config cfg;
    cfg.stack_size = 4096;

    osal::task t(sleep_entry, cfg);
    // should not be done immediately
    REQUIRE(g_task_ran.load() == 0);
    t.join();
    REQUIRE(g_task_ran.load() == 1);
}

TEST_CASE("task: joinable", "[task]") {
    int dummy = 0;
    osal::task::config cfg;
    cfg.stack_size = 4096;
    cfg.param = &dummy;

    osal::task t;
    REQUIRE(t.joinable() == false);
    t.create(task_entry, cfg);
    REQUIRE(t.joinable() == false);  // created but not started
    t.start();
    REQUIRE(t.joinable() == true);
    t.join();
    REQUIRE(t.joinable() == false);
}
