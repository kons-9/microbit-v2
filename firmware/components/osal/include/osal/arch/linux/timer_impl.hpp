#pragma once

#include <signal.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>
#include <cassert>
#include <atomic>

namespace osal {

namespace detail {
struct linux_cyclic_ctx {
    timer_t timerid;
    cyclic_timer::handler_t handler;
    void* param;
    uint32_t interval_ms;
    bool running;
};

inline void cyclic_signal_handler(union sigval sv) {
    auto* ctx = reinterpret_cast<linux_cyclic_ctx*>(sv.sival_ptr);
    if (ctx->handler) {
        ctx->handler(ctx->param);
    }
}

struct linux_oneshot_ctx {
    timer_t timerid;
    oneshot_timer::handler_t handler;
    void* param;
    bool running;
};

inline void oneshot_signal_handler(union sigval sv) {
    auto* ctx = reinterpret_cast<linux_oneshot_ctx*>(sv.sival_ptr);
    if (ctx->handler) {
        ctx->handler(ctx->param);
    }
    ctx->running = false;
}
} // namespace detail

// ===== cyclic_timer =====

inline cyclic_timer::cyclic_timer(handler_t handler, uint32_t interval_ms, void* param) {
    static_assert(sizeof(storage_) >= sizeof(detail::linux_cyclic_ctx),
                  "storage too small for linux_cyclic_ctx");
    auto* ctx = reinterpret_cast<detail::linux_cyclic_ctx*>(storage_);
    ctx->handler = handler;
    ctx->param = param;
    ctx->interval_ms = interval_ms;
    ctx->running = false;

    struct sigevent sev = {};
    sev.sigev_notify = SIGEV_THREAD;
    sev.sigev_notify_function = detail::cyclic_signal_handler;
    sev.sigev_value.sival_ptr = ctx;
    int rc = timer_create(CLOCK_REALTIME, &sev, &ctx->timerid);
    assert(rc == 0);
    (void)rc;
}

inline cyclic_timer::~cyclic_timer() {
    auto* ctx = reinterpret_cast<detail::linux_cyclic_ctx*>(storage_);
    stop();
    timer_delete(ctx->timerid);
}

inline void cyclic_timer::start() {
    auto* ctx = reinterpret_cast<detail::linux_cyclic_ctx*>(storage_);
    struct itimerspec its = {};
    its.it_value.tv_sec = ctx->interval_ms / 1000;
    its.it_value.tv_nsec = (ctx->interval_ms % 1000) * 1000000L;
    its.it_interval = its.it_value;
    timer_settime(ctx->timerid, 0, &its, nullptr);
    ctx->running = true;
}

inline void cyclic_timer::stop() {
    auto* ctx = reinterpret_cast<detail::linux_cyclic_ctx*>(storage_);
    if (ctx->running) {
        struct itimerspec its = {};
        timer_settime(ctx->timerid, 0, &its, nullptr);
        ctx->running = false;
    }
}

// ===== oneshot_timer =====

inline oneshot_timer::oneshot_timer(handler_t handler, void* param) {
    static_assert(sizeof(storage_) >= sizeof(detail::linux_oneshot_ctx),
                  "storage too small for linux_oneshot_ctx");
    auto* ctx = reinterpret_cast<detail::linux_oneshot_ctx*>(storage_);
    ctx->handler = handler;
    ctx->param = param;
    ctx->running = false;

    struct sigevent sev = {};
    sev.sigev_notify = SIGEV_THREAD;
    sev.sigev_notify_function = detail::oneshot_signal_handler;
    sev.sigev_value.sival_ptr = ctx;
    int rc = timer_create(CLOCK_REALTIME, &sev, &ctx->timerid);
    assert(rc == 0);
    (void)rc;
}

inline oneshot_timer::~oneshot_timer() {
    auto* ctx = reinterpret_cast<detail::linux_oneshot_ctx*>(storage_);
    stop();
    timer_delete(ctx->timerid);
}

inline void oneshot_timer::start(uint32_t delay_ms) {
    auto* ctx = reinterpret_cast<detail::linux_oneshot_ctx*>(storage_);
    struct itimerspec its = {};
    its.it_value.tv_sec = delay_ms / 1000;
    its.it_value.tv_nsec = (delay_ms % 1000) * 1000000L;
    // it_interval = 0 → one-shot
    timer_settime(ctx->timerid, 0, &its, nullptr);
    ctx->running = true;
}

inline void oneshot_timer::stop() {
    auto* ctx = reinterpret_cast<detail::linux_oneshot_ctx*>(storage_);
    if (ctx->running) {
        struct itimerspec its = {};
        timer_settime(ctx->timerid, 0, &its, nullptr);
        ctx->running = false;
    }
}

} // namespace osal
