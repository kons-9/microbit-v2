#pragma once

#include <pthread.h>
#include <unistd.h>
#include <sched.h>
#include <cassert>
#include <cstring>

namespace osal {

namespace detail {
struct linux_task_ctx {
    pthread_t thread;
    task::entry_t entry;
    void* param;
    bool joined;
};
} // namespace detail

inline task::task(entry_t entry, const config& cfg) {
    create(entry, cfg);
    start();
}

inline task::~task() {
    if (started_) {
        join();
    }
}

inline bool task::create(entry_t entry, const config& cfg) {
    static_assert(sizeof(storage_) >= sizeof(detail::linux_task_ctx),
                  "storage too small for linux_task_ctx");
    auto* ctx = reinterpret_cast<detail::linux_task_ctx*>(storage_);
    ctx->entry = entry;
    ctx->param = cfg.param;
    ctx->joined = false;
    created_ = true;
    started_ = false;
    return true;
}

inline bool task::start() {
    if (!created_ || started_) return false;
    auto* ctx = reinterpret_cast<detail::linux_task_ctx*>(storage_);

    auto wrapper = [](void* arg) -> void* {
        auto* c = reinterpret_cast<detail::linux_task_ctx*>(arg);
        c->entry(c->param);
        return nullptr;
    };

    int rc = pthread_create(&ctx->thread, nullptr, wrapper, ctx);
    if (rc != 0) return false;
    started_ = true;
    return true;
}

inline bool task::terminate() {
    if (!started_) return false;
    auto* ctx = reinterpret_cast<detail::linux_task_ctx*>(storage_);
    pthread_cancel(ctx->thread);
    pthread_join(ctx->thread, nullptr);
    ctx->joined = true;
    started_ = false;
    return true;
}

inline bool task::suspend() {
    // Linux has no direct suspend; no-op
    return started_;
}

inline bool task::resume() {
    // Linux has no direct resume; no-op
    return started_;
}

inline bool task::joinable() const {
    return started_;
}

inline void task::join() {
    if (!started_) return;
    auto* ctx = reinterpret_cast<detail::linux_task_ctx*>(storage_);
    if (!ctx->joined) {
        pthread_join(ctx->thread, nullptr);
        ctx->joined = true;
    }
    started_ = false;
}

inline void task::sleep_for(uint32_t ms) {
    usleep(static_cast<useconds_t>(ms) * 1000);
}

inline void task::yield() {
    sched_yield();
}

} // namespace osal
