#include <catch2/catch_test_macros.hpp>
#include <osal/mutex>
#include <pthread.h>

TEST_CASE("mutex: basic lock/unlock", "[mutex]") {
    osal::mutex mtx;
    mtx.lock();
    mtx.unlock();
}

TEST_CASE("mutex: try_lock", "[mutex]") {
    osal::mutex mtx;
    REQUIRE(mtx.try_lock() == true);
    REQUIRE(mtx.try_lock() == false);  // already locked
    mtx.unlock();
    REQUIRE(mtx.try_lock() == true);
    mtx.unlock();
}

TEST_CASE("mutex: lock_guard RAII", "[mutex]") {
    osal::mutex mtx;
    {
        osal::lock_guard<osal::mutex> lk(mtx);
        REQUIRE(mtx.try_lock() == false);
    }
    REQUIRE(mtx.try_lock() == true);
    mtx.unlock();
}

static osal::mutex g_mtx;
static int g_counter = 0;

static void *increment_thread(void *) {
    for (int i = 0; i < 100000; ++i) {
        osal::lock_guard<osal::mutex> lk(g_mtx);
        ++g_counter;
    }
    return nullptr;
}

TEST_CASE("mutex: multi-thread exclusion", "[mutex]") {
    constexpr int NUM_THREADS = 4;
    g_counter = 0;
    pthread_t threads[NUM_THREADS];

    for (int i = 0; i < NUM_THREADS; ++i) {
        pthread_create(&threads[i], nullptr, increment_thread, nullptr);
    }
    for (int i = 0; i < NUM_THREADS; ++i) {
        pthread_join(threads[i], nullptr);
    }

    REQUIRE(g_counter == NUM_THREADS * 100000);
}
