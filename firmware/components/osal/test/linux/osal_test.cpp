#include <cassert>
#include <cstdio>
#include <pthread.h>
#include <osal/mutex>

// 排他テスト: 複数スレッドがカウンタをインクリメント
static osal::mutex g_mtx;
static int g_counter = 0;
static constexpr int ITERATIONS = 100000;
static constexpr int NUM_THREADS = 4;

static void *thread_func(void *) {
    for (int i = 0; i < ITERATIONS; ++i) {
        osal::lock_guard<osal::mutex> lk(g_mtx);
        ++g_counter;
    }
    return nullptr;
}

static void test_mutex_exclusion() {
    pthread_t threads[NUM_THREADS];
    g_counter = 0;

    for (int i = 0; i < NUM_THREADS; ++i) {
        pthread_create(&threads[i], nullptr, thread_func, nullptr);
    }
    for (int i = 0; i < NUM_THREADS; ++i) {
        pthread_join(threads[i], nullptr);
    }

    assert(g_counter == NUM_THREADS * ITERATIONS);
    std::printf("[PASS] mutex exclusion: counter=%d\n", g_counter);
}

static void test_try_lock() {
    osal::mutex mtx;
    assert(mtx.try_lock() == true);
    // already locked by this thread — pthread default mutex: trylock returns EBUSY
    assert(mtx.try_lock() == false);
    mtx.unlock();
    // now available
    assert(mtx.try_lock() == true);
    mtx.unlock();
    std::printf("[PASS] try_lock\n");
}

static void test_lock_guard() {
    osal::mutex mtx;
    {
        osal::lock_guard<osal::mutex> lk(mtx);
        // should be locked — try_lock fails
        // Note: from same thread on default mutex, try_lock returns EBUSY
    }
    // after scope, should be unlocked
    assert(mtx.try_lock() == true);
    mtx.unlock();
    std::printf("[PASS] lock_guard\n");
}

int main() {
    test_mutex_exclusion();
    test_try_lock();
    test_lock_guard();
    std::printf("\nAll mutex tests passed!\n");
    return 0;
}
