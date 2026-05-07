#pragma once

#include "base.hpp"

namespace utk {

class Mutex : non_copyable {
public:
    struct Config {
        void*   exinf   = nullptr;
        ATR     attr    = TA_INHERIT;
        PRI     ceilpri = 0;
    };

    Mutex() = default;

    explicit Mutex(const Config& cfg) { create(cfg); }

    ~Mutex() { destroy(); }

    Mutex(Mutex&& o) : id_(o.id_) { o.id_ = 0; }
    Mutex& operator=(Mutex&& o) {
        if (this != &o) { destroy(); id_ = o.id_; o.id_ = 0; }
        return *this;
    }

    err_t create(const Config& cfg) {
        T_CMTX cmtx = {};
        cmtx.exinf   = cfg.exinf;
        cmtx.mtxatr  = cfg.attr;
        cmtx.ceilpri = cfg.ceilpri;
        ID ret = tk_cre_mtx(&cmtx);
        if (ret > 0) { id_ = ret; return E_OK; }
        return static_cast<err_t>(ret);
    }

    void destroy() {
        if (id_ > 0) { tk_del_mtx(id_); id_ = 0; }
    }

    err_t lock(TMO tmout = TMO_FEVR) { return tk_loc_mtx(id_, tmout); }
    err_t unlock()                    { return tk_unl_mtx(id_); }
    err_t ref(T_RMTX& rmtx) const    { return tk_ref_mtx(id_, &rmtx); }

    ID id() const { return id_; }
    bool valid() const { return id_ > 0; }

    // RAII lock guard
    class Guard {
    public:
        explicit Guard(Mutex& mtx, TMO tmout = TMO_FEVR)
            : mtx_(mtx) { mtx_.lock(tmout); }
        ~Guard() { mtx_.unlock(); }
        Guard(const Guard&) = delete;
        Guard& operator=(const Guard&) = delete;
    private:
        Mutex& mtx_;
    };

private:
    ID id_ = 0;
};

} // namespace utk
