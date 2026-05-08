#pragma once

#include <pthread.h>
#include <time.h>
#include <cassert>
#include <cerrno>

namespace osal {

namespace detail {
struct linux_eventflag {
    pthread_mutex_t mtx;
    pthread_cond_t cond;
    uint32_t flags;
};
}  // namespace detail

inline event_flag::event_flag(uint32_t initial) {
    static_assert(sizeof(m_storage) >= sizeof(detail::linux_eventflag), "storage too small");
    auto *ef = reinterpret_cast<detail::linux_eventflag *>(m_storage);
    pthread_mutex_init(&ef->mtx, nullptr);
    pthread_cond_init(&ef->cond, nullptr);
    ef->flags = initial;
}

inline event_flag::~event_flag() {
    auto *ef = reinterpret_cast<detail::linux_eventflag *>(m_storage);
    pthread_cond_destroy(&ef->cond);
    pthread_mutex_destroy(&ef->mtx);
}

inline void event_flag::set(uint32_t bits) {
    auto *ef = reinterpret_cast<detail::linux_eventflag *>(m_storage);
    pthread_mutex_lock(&ef->mtx);
    ef->flags |= bits;
    pthread_cond_broadcast(&ef->cond);
    pthread_mutex_unlock(&ef->mtx);
}

inline void event_flag::clear(uint32_t bits) {
    auto *ef = reinterpret_cast<detail::linux_eventflag *>(m_storage);
    pthread_mutex_lock(&ef->mtx);
    ef->flags &= ~bits;
    pthread_mutex_unlock(&ef->mtx);
}

inline uint32_t event_flag::wait(uint32_t pattern, wait_mode mode, uint32_t timeout_ms) {
    auto *ef = reinterpret_cast<detail::linux_eventflag *>(m_storage);
    pthread_mutex_lock(&ef->mtx);

    struct timespec ts;
    bool has_timeout = (timeout_ms != UINT32_MAX);
    if (has_timeout) {
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_sec += timeout_ms / 1000;
        ts.tv_nsec += (timeout_ms % 1000) * 1000000L;
        if (ts.tv_nsec >= 1000000000L) {
            ts.tv_sec += 1;
            ts.tv_nsec -= 1000000000L;
        }
    }

    while (true) {
        bool match = (mode == any) ? (ef->flags & pattern) != 0 : (ef->flags & pattern) == pattern;
        if (match) {
            uint32_t result = ef->flags & pattern;
            pthread_mutex_unlock(&ef->mtx);
            return result;
        }
        if (has_timeout) {
            int rc = pthread_cond_timedwait(&ef->cond, &ef->mtx, &ts);
            if (rc == ETIMEDOUT) {
                pthread_mutex_unlock(&ef->mtx);
                return 0;
            }
        } else {
            pthread_cond_wait(&ef->cond, &ef->mtx);
        }
    }
}

inline uint32_t event_flag::get() const {
    auto *ef = const_cast<detail::linux_eventflag *>(reinterpret_cast<const detail::linux_eventflag *>(m_storage));
    pthread_mutex_lock(&ef->mtx);
    uint32_t val = ef->flags;
    pthread_mutex_unlock(&ef->mtx);
    return val;
}

}  // namespace osal
