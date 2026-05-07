#pragma once

#include <tk/tkernel.h>
#include <cassert>

namespace osal {

namespace detail {
struct microbit_task_ctx {
    ID id;
    task::entry_t entry;
    void *param;
};

inline void task_entry_wrapper(INT stacd, void *exinf) {
    auto *ctx = reinterpret_cast<microbit_task_ctx *>(exinf);
    ctx->entry(ctx->param);
    tk_ext_tsk();  // タスク終了
}
}  // namespace detail

inline task::task(entry_t entry, const config &cfg) {
    create(entry, cfg);
    start();
}

inline task::~task() {
    if (started_ || created_) {
        terminate();
    }
}

inline bool task::create(entry_t entry, const config &cfg) {
    static_assert(sizeof(storage_) >= sizeof(detail::microbit_task_ctx), "storage too small for microbit_task_ctx");
    auto *ctx = reinterpret_cast<detail::microbit_task_ctx *>(storage_);
    ctx->entry = entry;
    ctx->param = cfg.param;

    T_CTSK ctsk = {};
    ctsk.exinf = ctx;
    ctsk.tskatr = TA_HLNG | TA_RNG0;
    ctsk.task = detail::task_entry_wrapper;
    ctsk.itskpri = static_cast<PRI>(cfg.priority);
    ctsk.stksz = static_cast<SZ>(cfg.stack_size);

    ID id = tk_cre_tsk(&ctsk);
    if (id <= 0)
        return false;
    ctx->id = id;
    created_ = true;
    started_ = false;
    return true;
}

inline bool task::start() {
    if (!created_ || started_)
        return false;
    auto *ctx = reinterpret_cast<detail::microbit_task_ctx *>(storage_);
    ER rc = tk_sta_tsk(ctx->id, 0);
    if (rc != E_OK)
        return false;
    started_ = true;
    return true;
}

inline bool task::terminate() {
    if (!created_)
        return false;
    auto *ctx = reinterpret_cast<detail::microbit_task_ctx *>(storage_);
    if (started_) {
        tk_ter_tsk(ctx->id);
        started_ = false;
    }
    tk_del_tsk(ctx->id);
    ctx->id = 0;
    created_ = false;
    return true;
}

inline bool task::suspend() {
    if (!started_)
        return false;
    auto *ctx = reinterpret_cast<detail::microbit_task_ctx *>(storage_);
    return tk_sus_tsk(ctx->id) == E_OK;
}

inline bool task::resume() {
    if (!started_)
        return false;
    auto *ctx = reinterpret_cast<detail::microbit_task_ctx *>(storage_);
    return tk_rsm_tsk(ctx->id) == E_OK;
}

inline bool task::joinable() const {
    return started_;
}

inline void task::join() {
    // uT-Kernel doesn't have a native join; poll task state
    if (!started_)
        return;
    auto *ctx = reinterpret_cast<detail::microbit_task_ctx *>(storage_);
    T_RTSK rtsk;
    while (true) {
        ER rc = tk_ref_tsk(ctx->id, &rtsk);
        if (rc != E_OK || rtsk.tskstat == TTS_DMT)
            break;
        tk_dly_tsk(1);
    }
    started_ = false;
}

inline void task::sleep_for(uint32_t ms) {
    tk_dly_tsk(static_cast<RELTIM>(ms));
}

inline void task::yield() {
    tk_dly_tsk(0);
}

}  // namespace osal
