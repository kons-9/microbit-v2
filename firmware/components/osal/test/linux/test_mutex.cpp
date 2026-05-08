#include <catch2/catch_test_macros.hpp>
#include <osal/mutex>

TEST_CASE("mutex lock/unlock", "[mutex]") {
    osal::mutex mtx;
    mtx.lock();
    mtx.unlock();
}

TEST_CASE("mutex try_lock", "[mutex]") {
    osal::mutex mtx;
    REQUIRE(mtx.try_lock());
    mtx.unlock();
}

TEST_CASE("lock_guard", "[mutex]") {
    osal::mutex mtx;
    { osal::lock_guard<osal::mutex> guard(mtx); }
    REQUIRE(mtx.try_lock());
    mtx.unlock();
}
