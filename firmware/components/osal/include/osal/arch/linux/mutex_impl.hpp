#pragma once

#include <pthread.h>
#include <cassert>

namespace osal {

inline mutex::mutex() {
    static_assert(sizeof(m_storage) >= sizeof(pthread_mutex_t), "storage too small");
    auto *mtx = reinterpret_cast<pthread_mutex_t *>(m_storage);
    int rc = pthread_mutex_init(mtx, nullptr);
    assert(rc == 0);
    (void)rc;
}

inline mutex::~mutex() {
    auto *mtx = reinterpret_cast<pthread_mutex_t *>(m_storage);
    pthread_mutex_destroy(mtx);
}

inline void mutex::lock() {
    auto *mtx = reinterpret_cast<pthread_mutex_t *>(m_storage);
    int rc = pthread_mutex_lock(mtx);
    assert(rc == 0);
    (void)rc;
}

inline bool mutex::try_lock() {
    auto *mtx = reinterpret_cast<pthread_mutex_t *>(m_storage);
    return pthread_mutex_trylock(mtx) == 0;
}

inline void mutex::unlock() {
    auto *mtx = reinterpret_cast<pthread_mutex_t *>(m_storage);
    int rc = pthread_mutex_unlock(mtx);
    assert(rc == 0);
    (void)rc;
}

}  // namespace osal
