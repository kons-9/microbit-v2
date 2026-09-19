#pragma once

#include <tk/tkernel.h>
#include <cassert>

namespace utkernel {

namespace detail {
struct microbit_task_ctx {
    ID id;
    task::entry_t entry;
    void *param;
};

inline void task_entry_wrapper(INT stacd, void *exinf) {
    (void)stacd;
    auto *ctx = reinterpret_cast<microbit_task_ctx *>(exinf);
    ctx->entry(ctx->param);
    tk_ext_tsk();
}
}  // namespace detail

inline task::task(entry_t entry, const config &cfg) {
    create(entry, cfg);
    start();
}

inline task::~task() {
    if (m_started || m_created) {
        terminate();
    }
}

inline bool task::create(entry_t entry, const config &cfg) {
    static_assert(sizeof(m_storage) >= sizeof(detail::microbit_task_ctx), "storage too small");
    auto *ctx = reinterpret_cast<detail::microbit_task_ctx *>(m_storage);
    ctx->entry = entry;
    ctx->param = cfg.param;

    T_CTSK ctsk = {};
    ctsk.exinf = ctx;
    ctsk.tskatr = TA_HLNG | TA_RNG0;
    ctsk.task = reinterpret_cast<FP>(detail::task_entry_wrapper);
    ctsk.itskpri = static_cast<PRI>(cfg.priority);
    ctsk.stksz = static_cast<SZ>(cfg.stack_size);

    ID id = tk_cre_tsk(&ctsk);
    if (id <= 0) {
        return false;
    }
    ctx->id = id;
    m_created = true;
    m_started = false;
    return true;
}

inline bool task::start() {
    if (!m_created || m_started) {
        return false;
    }
    auto *ctx = reinterpret_cast<detail::microbit_task_ctx *>(m_storage);
    ER rc = tk_sta_tsk(ctx->id, 0);
    if (rc != E_OK) {
        return false;
    }
    m_started = true;
    return true;
}

inline bool task::terminate() {
    if (!m_created) {
        return false;
    }
    auto *ctx = reinterpret_cast<detail::microbit_task_ctx *>(m_storage);
    if (m_started) {
        tk_ter_tsk(ctx->id);
        m_started = false;
    }
    tk_del_tsk(ctx->id);
    ctx->id = 0;
    m_created = false;
    return true;
}

inline bool task::suspend() {
    if (!m_started) {
        return false;
    }
    auto *ctx = reinterpret_cast<detail::microbit_task_ctx *>(m_storage);
    return tk_sus_tsk(ctx->id) == E_OK;
}

inline bool task::resume() {
    if (!m_started) {
        return false;
    }
    auto *ctx = reinterpret_cast<detail::microbit_task_ctx *>(m_storage);
    return tk_rsm_tsk(ctx->id) == E_OK;
}

inline bool task::joinable() const {
    return m_started;
}

inline void task::join() {
    if (!m_started) {
        return;
    }
    auto *ctx = reinterpret_cast<detail::microbit_task_ctx *>(m_storage);
    T_RTSK rtsk;
    while (true) {
        ER rc = tk_ref_tsk(ctx->id, &rtsk);
        if (rc != E_OK || rtsk.tskstat == TTS_DMT) {
            break;
        }
        tk_dly_tsk(1);
    }
    m_started = false;
}

inline void task::sleep_for(uint32_t ms) {
    tk_dly_tsk(static_cast<RELTIM>(ms));
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
    tk_dly_tsk(0);
}

}  // namespace utkernel
