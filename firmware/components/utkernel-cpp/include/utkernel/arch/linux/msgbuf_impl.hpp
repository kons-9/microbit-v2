#pragma once

#include <pthread.h>
#include <time.h>
#include <cstring>
#include <cassert>
#include <cerrno>
#include <cstdlib>

namespace utkernel {

namespace detail {
struct linux_msgbuf {
    pthread_mutex_t mtx;
    pthread_cond_t cond_send;
    pthread_cond_t cond_recv;
    uint8_t *buffer;
    size_t buf_size;
    size_t max_msg_size;
    size_t head;
    size_t tail;
    size_t count;
};

inline void timespec_from_ms(struct timespec &ts, uint32_t timeout_ms) {
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += timeout_ms / 1000;
    ts.tv_nsec += (timeout_ms % 1000) * 1000000L;
    if (ts.tv_nsec >= 1000000000L) {
        ts.tv_sec += 1;
        ts.tv_nsec -= 1000000000L;
    }
}
}  // namespace detail

inline message_buffer::message_buffer(size_t buf_size, size_t max_msg_size) {
    static_assert(sizeof(m_storage) >= sizeof(detail::linux_msgbuf), "storage too small");
    auto *mb = reinterpret_cast<detail::linux_msgbuf *>(m_storage);
    pthread_mutex_init(&mb->mtx, nullptr);
    pthread_cond_init(&mb->cond_send, nullptr);
    pthread_cond_init(&mb->cond_recv, nullptr);
    mb->buffer = static_cast<uint8_t *>(std::malloc(buf_size));
    assert(mb->buffer != nullptr);
    mb->buf_size = buf_size;
    mb->max_msg_size = max_msg_size;
    mb->head = 0;
    mb->tail = 0;
    mb->count = 0;
}

inline message_buffer::~message_buffer() {
    auto *mb = reinterpret_cast<detail::linux_msgbuf *>(m_storage);
    std::free(mb->buffer);
    pthread_cond_destroy(&mb->cond_recv);
    pthread_cond_destroy(&mb->cond_send);
    pthread_mutex_destroy(&mb->mtx);
}

inline bool message_buffer::send(const void *data, size_t size, uint32_t timeout_ms) {
    auto *mb = reinterpret_cast<detail::linux_msgbuf *>(m_storage);
    if (size == 0 || size > mb->max_msg_size) {
        return false;
    }

    size_t frame_size = sizeof(uint32_t) + size;

    pthread_mutex_lock(&mb->mtx);

    bool has_timeout = (timeout_ms != UINT32_MAX);
    struct timespec ts;
    if (has_timeout) {
        detail::timespec_from_ms(ts, timeout_ms);
    }

    while (mb->buf_size - mb->count < frame_size) {
        if (has_timeout) {
            int rc = pthread_cond_timedwait(&mb->cond_send, &mb->mtx, &ts);
            if (rc == ETIMEDOUT) {
                pthread_mutex_unlock(&mb->mtx);
                return false;
            }
        } else {
            pthread_cond_wait(&mb->cond_send, &mb->mtx);
        }
    }

    auto write_byte = [&](uint8_t b) {
        mb->buffer[mb->tail] = b;
        mb->tail = (mb->tail + 1) % mb->buf_size;
        mb->count++;
    };

    auto len = static_cast<uint32_t>(size);
    for (size_t i = 0; i < sizeof(uint32_t); ++i) {
        write_byte(static_cast<uint8_t>((len >> (i * 8)) & 0xFF));
    }
    const auto *src = static_cast<const uint8_t *>(data);
    for (size_t i = 0; i < size; ++i) {
        write_byte(src[i]);
    }

    pthread_cond_signal(&mb->cond_recv);
    pthread_mutex_unlock(&mb->mtx);
    return true;
}

inline size_t message_buffer::receive(void *data, size_t max_size, uint32_t timeout_ms) {
    auto *mb = reinterpret_cast<detail::linux_msgbuf *>(m_storage);

    pthread_mutex_lock(&mb->mtx);

    bool has_timeout = (timeout_ms != UINT32_MAX);
    struct timespec ts;
    if (has_timeout) {
        detail::timespec_from_ms(ts, timeout_ms);
    }

    while (mb->count < sizeof(uint32_t)) {
        if (has_timeout) {
            int rc = pthread_cond_timedwait(&mb->cond_recv, &mb->mtx, &ts);
            if (rc == ETIMEDOUT) {
                pthread_mutex_unlock(&mb->mtx);
                return 0;
            }
        } else {
            pthread_cond_wait(&mb->cond_recv, &mb->mtx);
        }
    }

    auto read_byte = [&]() -> uint8_t {
        uint8_t b = mb->buffer[mb->head];
        mb->head = (mb->head + 1) % mb->buf_size;
        mb->count--;
        return b;
    };

    uint32_t len = 0;
    for (size_t i = 0; i < sizeof(uint32_t); ++i) {
        len |= static_cast<uint32_t>(read_byte()) << (i * 8);
    }

    size_t to_read = (len <= max_size) ? len : max_size;
    auto *dst = static_cast<uint8_t *>(data);
    for (size_t i = 0; i < len; ++i) {
        uint8_t b = read_byte();
        if (i < to_read) {
            dst[i] = b;
        }
    }

    pthread_cond_signal(&mb->cond_send);
    pthread_mutex_unlock(&mb->mtx);
    return to_read;
}

}  // namespace utkernel
