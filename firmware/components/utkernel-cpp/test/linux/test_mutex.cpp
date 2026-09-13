#include <catch2/catch_test_macros.hpp>
#include <utkernel/mutex>

TEST_CASE("mutex lock/unlock", "[mutex]") {
    utkernel::mutex mtx;
    mtx.lock();
    mtx.unlock();
}

TEST_CASE("mutex try_lock", "[mutex]") {
    utkernel::mutex mtx;
    REQUIRE(mtx.try_lock());
    mtx.unlock();
}

TEST_CASE("lock_guard", "[mutex]") {
    utkernel::mutex mtx;
    { utkernel::lock_guard<utkernel::mutex> guard(mtx); }
    REQUIRE(mtx.try_lock());
    mtx.unlock();
}
