#pragma once

#include "base.hpp"

namespace utk {

class AlarmHandler : non_copyable {
public:
    using func_t = void (*)(void* exinf);

    struct Config {
        void*   exinf   = nullptr;
        ATR     attr    = TA_HLNG;
        func_t  handler = nullptr;
    };

    AlarmHandler() = default;

    explicit AlarmHandler(const Config& cfg) { create(cfg); }

    ~AlarmHandler() { destroy(); }

    AlarmHandler(AlarmHandler&& o) : id_(o.id_) { o.id_ = 0; }
    AlarmHandler& operator=(AlarmHandler&& o) {
        if (this != &o) { destroy(); id_ = o.id_; o.id_ = 0; }
        return *this;
    }

    err_t create(const Config& cfg) {
        T_CALM calm = {};
        calm.exinf   = cfg.exinf;
        calm.almatr  = cfg.attr;
        calm.almhdr  = reinterpret_cast<FP>(cfg.handler);
        ID ret = tk_cre_alm(&calm);
        if (ret > 0) { id_ = ret; return E_OK; }
        return static_cast<err_t>(ret);
    }

    void destroy() {
        if (id_ > 0) { tk_del_alm(id_); id_ = 0; }
    }

    err_t start(RELTIM timeout) { return tk_sta_alm(id_, timeout); }
    err_t stop()                { return tk_stp_alm(id_); }
    err_t ref(T_RALM& ralm) const { return tk_ref_alm(id_, &ralm); }

    ID id() const { return id_; }
    bool valid() const { return id_ > 0; }

private:
    ID id_ = 0;
};

} // namespace utk
