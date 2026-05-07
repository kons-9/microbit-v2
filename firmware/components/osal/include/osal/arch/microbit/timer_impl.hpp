#pragma once

#include <tk/tkernel.h>
#include <cassert>

namespace osal {

namespace detail {
struct microbit_cyclic_ctx {
    ID id;
};

struct microbit_oneshot_ctx {
    ID id;
};
} // namespace detail

// ===== cyclic_timer =====

inline cyclic_timer::cyclic_timer(handler_t handler, uint32_t interval_ms, void* param) {
    static_assert(sizeof(storage_) >= sizeof(detail::microbit_cyclic_ctx),
                  "storage too small for microbit_cyclic_ctx");
    auto* ctx = reinterpret_cast<detail::microbit_cyclic_ctx*>(storage_);

    T_CCYC ccyc = {};
    ccyc.exinf   = param;
    ccyc.cycatr  = TA_HLNG | TA_STA;  // auto-start disabled initially
    ccyc.cychdr  = reinterpret_cast<FP>(handler);
    ccyc.cyctim  = static_cast<RELTIM>(interval_ms);
    ccyc.cycphs  = 0;

    // Create stopped (remove TA_STA)
    ccyc.cycatr = TA_HLNG;
    ID id = tk_cre_cyc(&ccyc);
    assert(id > 0);
    ctx->id = id;
}

inline cyclic_timer::~cyclic_timer() {
    auto* ctx = reinterpret_cast<detail::microbit_cyclic_ctx*>(storage_);
    if (ctx->id > 0) {
        tk_stp_cyc(ctx->id);
        tk_del_cyc(ctx->id);
    }
}

inline void cyclic_timer::start() {
    auto* ctx = reinterpret_cast<detail::microbit_cyclic_ctx*>(storage_);
    tk_sta_cyc(ctx->id);
}

inline void cyclic_timer::stop() {
    auto* ctx = reinterpret_cast<detail::microbit_cyclic_ctx*>(storage_);
    tk_stp_cyc(ctx->id);
}

// ===== oneshot_timer =====

inline oneshot_timer::oneshot_timer(handler_t handler, void* param) {
    static_assert(sizeof(storage_) >= sizeof(detail::microbit_oneshot_ctx),
                  "storage too small for microbit_oneshot_ctx");
    auto* ctx = reinterpret_cast<detail::microbit_oneshot_ctx*>(storage_);

    T_CALM calm = {};
    calm.exinf  = param;
    calm.almatr = TA_HLNG;
    calm.almhdr = reinterpret_cast<FP>(handler);

    ID id = tk_cre_alm(&calm);
    assert(id > 0);
    ctx->id = id;
}

inline oneshot_timer::~oneshot_timer() {
    auto* ctx = reinterpret_cast<detail::microbit_oneshot_ctx*>(storage_);
    if (ctx->id > 0) {
        tk_stp_alm(ctx->id);
        tk_del_alm(ctx->id);
    }
}

inline void oneshot_timer::start(uint32_t delay_ms) {
    auto* ctx = reinterpret_cast<detail::microbit_oneshot_ctx*>(storage_);
    tk_sta_alm(ctx->id, static_cast<RELTIM>(delay_ms));
}

inline void oneshot_timer::stop() {
    auto* ctx = reinterpret_cast<detail::microbit_oneshot_ctx*>(storage_);
    tk_stp_alm(ctx->id);
}

} // namespace osal
