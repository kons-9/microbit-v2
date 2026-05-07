#pragma once

#include "base.hpp"

namespace utk {

class CyclicHandler : non_copyable {
public:
    using func_t = void (*)(void* exinf);

    struct Config {
        void*   exinf   = nullptr;
        ATR     attr    = TA_HLNG | TA_STA;
        func_t  handler = nullptr;
        RELTIM  interval = 1000;
        RELTIM  phase   = 0;
    };

    CyclicHandler() = default;

    explicit CyclicHandler(const Config& cfg) { create(cfg); }

    ~CyclicHandler() { destroy(); }

    CyclicHandler(CyclicHandler&& o) : id_(o.id_) { o.id_ = 0; }
    CyclicHandler& operator=(CyclicHandler&& o) {
        if (this != &o) { destroy(); id_ = o.id_; o.id_ = 0; }
        return *this;
    }

    err_t create(const Config& cfg) {
        T_CCYC ccyc = {};
        ccyc.exinf   = cfg.exinf;
        ccyc.cycatr  = cfg.attr;
        ccyc.cychdr  = reinterpret_cast<FP>(cfg.handler);
        ccyc.cyctim  = cfg.interval;
        ccyc.cycphs  = cfg.phase;
        ID ret = tk_cre_cyc(&ccyc);
        if (ret > 0) { id_ = ret; return E_OK; }
        return static_cast<err_t>(ret);
    }

    void destroy() {
        if (id_ > 0) { tk_del_cyc(id_); id_ = 0; }
    }

    err_t start() { return tk_sta_cyc(id_); }
    err_t stop()  { return tk_stp_cyc(id_); }
    err_t ref(T_RCYC& rcyc) const { return tk_ref_cyc(id_, &rcyc); }

    ID id() const { return id_; }
    bool valid() const { return id_ > 0; }

private:
    ID id_ = 0;
};

} // namespace utk
