#include <catch2/catch_test_macros.hpp>
#include <utkernel/semaphore>

TEST_CASE("counting_semaphore acquire/release", "[semaphore]") {
    utkernel::counting_semaphore<10> sem(1);
    sem.acquire();
    sem.release();
}

TEST_CASE("counting_semaphore try_acquire", "[semaphore]") {
    utkernel::counting_semaphore<10> sem(0);
    REQUIRE_FALSE(sem.try_acquire());
    sem.release();
    REQUIRE(sem.try_acquire());
}

TEST_CASE("counting_semaphore try_acquire_for timeout", "[semaphore]") {
    utkernel::counting_semaphore<10> sem(0);
    REQUIRE_FALSE(sem.try_acquire_for(10));
}

TEST_CASE("binary_semaphore", "[semaphore]") {
    utkernel::binary_semaphore bsem(1);
    bsem.acquire();
    REQUIRE_FALSE(bsem.try_acquire());
    bsem.release();
    REQUIRE(bsem.try_acquire());
}
