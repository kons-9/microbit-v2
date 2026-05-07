#pragma once

#include <semaphore.h>
#include <time.h>
#include <cassert>
#include <cerrno>

namespace osal {

template<int LeastMaxValue>
inline counting_semaphore<LeastMaxValue>::counting_semaphore(int initial) {
    static_assert(sizeof(storage_) >= sizeof(sem_t), "storage too small for sem_t");
    auto* sem = reinterpret_cast<sem_t*>(storage_);
    int rc = sem_init(sem, 0, static_cast<unsigned>(initial));
    assert(rc == 0);
    (void)rc;
}

template<int LeastMaxValue>
inline counting_semaphore<LeastMaxValue>::~counting_semaphore() {
    auto* sem = reinterpret_cast<sem_t*>(storage_);
    sem_destroy(sem);
}

template<int LeastMaxValue>
inline void counting_semaphore<LeastMaxValue>::acquire() {
    auto* sem = reinterpret_cast<sem_t*>(storage_);
    while (sem_wait(sem) != 0) {
        assert(errno == EINTR);
    }
}

template<int LeastMaxValue>
inline bool counting_semaphore<LeastMaxValue>::try_acquire() {
    auto* sem = reinterpret_cast<sem_t*>(storage_);
    return sem_trywait(sem) == 0;
}

template<int LeastMaxValue>
inline bool counting_semaphore<LeastMaxValue>::try_acquire_for(uint32_t timeout_ms) {
    auto* sem = reinterpret_cast<sem_t*>(storage_);
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += timeout_ms / 1000;
    ts.tv_nsec += (timeout_ms % 1000) * 1000000L;
    if (ts.tv_nsec >= 1000000000L) {
        ts.tv_sec += 1;
        ts.tv_nsec -= 1000000000L;
    }
    while (sem_timedwait(sem, &ts) != 0) {
        if (errno == ETIMEDOUT) return false;
        assert(errno == EINTR);
    }
    return true;
}

template<int LeastMaxValue>
inline void counting_semaphore<LeastMaxValue>::release(int update) {
    auto* sem = reinterpret_cast<sem_t*>(storage_);
    for (int i = 0; i < update; ++i) {
        int rc = sem_post(sem);
        assert(rc == 0);
        (void)rc;
    }
}

} // namespace osal
