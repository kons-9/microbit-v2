#pragma once

#include "base.hpp"

namespace utk {

class Semaphore : non_copyable {
public:
    struct Config {
        void*   exinf   = nullptr;
        ATR     attr    = TA_TFIFO;
        INT     initial = 1;
        INT     max     = 1;
    };

    Semaphore() = default;

    explicit Semaphore(const Config& cfg) { create(cfg); }

    ~Semaphore() { destroy(); }

    Semaphore(Semaphore&& o) : id_(o.id_) { o.id_ = 0; }
    Semaphore& operator=(Semaphore&& o) {
        if (this != &o) { destroy(); id_ = o.id_; o.id_ = 0; }
        return *this;
    }

    err_t create(const Config& cfg) {
        T_CSEM csem = {};
        csem.exinf   = cfg.exinf;
        csem.sematr  = cfg.attr;
        csem.isemcnt = cfg.initial;
        csem.maxsem  = cfg.max;
        ID ret = tk_cre_sem(&csem);
        if (ret > 0) { id_ = ret; return E_OK; }
        return static_cast<err_t>(ret);
    }

    void destroy() {
        if (id_ > 0) { tk_del_sem(id_); id_ = 0; }
    }

    err_t signal(INT cnt = 1)             { return tk_sig_sem(id_, cnt); }
    err_t wait(INT cnt = 1, TMO tmout = TMO_FEVR) { return tk_wai_sem(id_, cnt, tmout); }
    err_t ref(T_RSEM& rsem) const         { return tk_ref_sem(id_, &rsem); }

    ID id() const { return id_; }
    bool valid() const { return id_ > 0; }

private:
    ID id_ = 0;
};

} // namespace utk
