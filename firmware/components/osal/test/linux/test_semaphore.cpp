#include <catch2/catch_test_macros.hpp>
#include <osal/semaphore>
#include <pthread.h>

TEST_CASE("semaphore: basic acquire/release", "[semaphore]") {
    osal::counting_semaphore<10> sem(1);
    sem.acquire();
    REQUIRE(sem.try_acquire() == false);  // count is 0
    sem.release();
    REQUIRE(sem.try_acquire() == true);  // count was 1
    sem.release();
}

TEST_CASE("semaphore: counting behavior", "[semaphore]") {
    osal::counting_semaphore<10> sem(3);
    REQUIRE(sem.try_acquire() == true);   // 3→2
    REQUIRE(sem.try_acquire() == true);   // 2→1
    REQUIRE(sem.try_acquire() == true);   // 1→0
    REQUIRE(sem.try_acquire() == false);  // 0: blocked
    sem.release(2);
    REQUIRE(sem.try_acquire() == true);  // 2→1
    REQUIRE(sem.try_acquire() == true);  // 1→0
    REQUIRE(sem.try_acquire() == false);
}

TEST_CASE("semaphore: try_acquire_for timeout", "[semaphore]") {
    osal::counting_semaphore<10> sem(0);
    REQUIRE(sem.try_acquire_for(50) == false);  // should timeout in ~50ms
}

TEST_CASE("semaphore: binary_semaphore", "[semaphore]") {
    osal::binary_semaphore bsem(1);
    REQUIRE(bsem.try_acquire() == true);
    REQUIRE(bsem.try_acquire() == false);
    bsem.release();
    REQUIRE(bsem.try_acquire() == true);
}

static osal::counting_semaphore<100> g_sem(0);
static int g_produced = 0;
static int g_consumed = 0;

static void *producer(void *) {
    for (int i = 0; i < 1000; ++i) {
        __atomic_add_fetch(&g_produced, 1, __ATOMIC_SEQ_CST);
        g_sem.release();
    }
    return nullptr;
}

static void *consumer(void *) {
    for (int i = 0; i < 1000; ++i) {
        g_sem.acquire();
        __atomic_add_fetch(&g_consumed, 1, __ATOMIC_SEQ_CST);
    }
    return nullptr;
}

TEST_CASE("semaphore: producer-consumer", "[semaphore]") {
    g_produced = 0;
    g_consumed = 0;
    pthread_t prod, cons;
    pthread_create(&prod, nullptr, producer, nullptr);
    pthread_create(&cons, nullptr, consumer, nullptr);
    pthread_join(prod, nullptr);
    pthread_join(cons, nullptr);
    REQUIRE(g_produced == 1000);
    REQUIRE(g_consumed == 1000);
}
