#pragma once

#include "base.hpp"

namespace utk {

class EventFlag : non_copyable {
public:
    struct Config {
        void*   exinf   = nullptr;
        ATR     attr    = TA_TFIFO | TA_WMUL;
        UINT    initial = 0;
    };

    EventFlag() = default;

    explicit EventFlag(const Config& cfg) { create(cfg); }

    ~EventFlag() { destroy(); }

    EventFlag(EventFlag&& o) : id_(o.id_) { o.id_ = 0; }
    EventFlag& operator=(EventFlag&& o) {
        if (this != &o) { destroy(); id_ = o.id_; o.id_ = 0; }
        return *this;
    }

    err_t create(const Config& cfg) {
        T_CFLG cflg = {};
        cflg.exinf   = cfg.exinf;
        cflg.flgatr  = cfg.attr;
        cflg.iflgptn = cfg.initial;
        ID ret = tk_cre_flg(&cflg);
        if (ret > 0) { id_ = ret; return E_OK; }
        return static_cast<err_t>(ret);
    }

    void destroy() {
        if (id_ > 0) { tk_del_flg(id_); id_ = 0; }
    }

    err_t set(UINT pattern)   { return tk_set_flg(id_, pattern); }
    err_t clear(UINT pattern) { return tk_clr_flg(id_, pattern); }

    err_t wait(UINT waiptn, UINT mode, UINT& result, TMO tmout = TMO_FEVR) {
        return tk_wai_flg(id_, waiptn, mode, &result, tmout);
    }

    err_t ref(T_RFLG& rflg) const { return tk_ref_flg(id_, &rflg); }

    ID id() const { return id_; }
    bool valid() const { return id_ > 0; }

private:
    ID id_ = 0;
};

} // namespace utk
