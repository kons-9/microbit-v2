#pragma once

#include <pthread.h>
#include <unistd.h>
#include <sched.h>
#include <cassert>

namespace utkernel {

namespace detail {
struct linux_task_ctx {
    pthread_t thread;
    task::entry_t entry;
    void *param;
    bool joined;
};
}  // namespace detail

inline task::task(entry_t entry, const config &cfg) {
    create(entry, cfg);
    start();
}

inline task::~task() {
    if (m_started) {
        join();
    }
}

inline bool task::create(entry_t entry, const config &cfg) {
    static_assert(sizeof(m_storage) >= sizeof(detail::linux_task_ctx), "storage too small");
    auto *ctx = reinterpret_cast<detail::linux_task_ctx *>(m_storage);
    ctx->entry = entry;
    ctx->param = cfg.param;
    ctx->joined = false;
    m_created = true;
    m_started = false;
    return true;
}

inline bool task::start() {
    if (!m_created || m_started) {
        return false;
    }
    auto *ctx = reinterpret_cast<detail::linux_task_ctx *>(m_storage);

    auto wrapper = [](void *arg) -> void * {
        auto *c = reinterpret_cast<detail::linux_task_ctx *>(arg);
        c->entry(c->param);
        return nullptr;
    };

    int rc = pthread_create(&ctx->thread, nullptr, wrapper, ctx);
    if (rc != 0) {
        return false;
    }
    m_started = true;
    return true;
}

inline bool task::terminate() {
    if (!m_started) {
        return false;
    }
    auto *ctx = reinterpret_cast<detail::linux_task_ctx *>(m_storage);
    pthread_cancel(ctx->thread);
    pthread_join(ctx->thread, nullptr);
    ctx->joined = true;
    m_started = false;
    return true;
}

inline bool task::suspend() {
    return m_started;
}

inline bool task::resume() {
    return m_started;
}

inline bool task::joinable() const {
    return m_started;
}

inline void task::join() {
    if (!m_started) {
        return;
    }
    auto *ctx = reinterpret_cast<detail::linux_task_ctx *>(m_storage);
    if (!ctx->joined) {
        pthread_join(ctx->thread, nullptr);
        ctx->joined = true;
    }
    m_started = false;
}

inline void task::sleep_for(uint32_t ms) {
    usleep(static_cast<useconds_t>(ms) * 1000);
}

inline void task::sleep_forever() {
#if defined(UTKERNEL_ARCH_microbit)
    tk_slp_tsk(TMO_FEVR);
#else
    for (;;) {
        sleep_for(60 * 60 * 1000);
    }
#endif
}

inline void task::yield() {
    sched_yield();
}

}  // namespace utkernel
