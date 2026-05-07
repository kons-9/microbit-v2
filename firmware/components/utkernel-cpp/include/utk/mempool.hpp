#pragma once

#include "base.hpp"

namespace utk {

class FixedMemPool : non_copyable {
public:
    struct Config {
        void*   exinf   = nullptr;
        ATR     attr    = TA_TFIFO;
        SZ      count   = 16;
        SZ      blksz   = 64;
    };

    FixedMemPool() = default;

    explicit FixedMemPool(const Config& cfg) { create(cfg); }

    ~FixedMemPool() { destroy(); }

    FixedMemPool(FixedMemPool&& o) : id_(o.id_) { o.id_ = 0; }
    FixedMemPool& operator=(FixedMemPool&& o) {
        if (this != &o) { destroy(); id_ = o.id_; o.id_ = 0; }
        return *this;
    }

    err_t create(const Config& cfg) {
        T_CMPF cmpf = {};
        cmpf.exinf   = cfg.exinf;
        cmpf.mpfatr  = cfg.attr;
        cmpf.mpfcnt  = cfg.count;
        cmpf.blfsz   = cfg.blksz;
        ID ret = tk_cre_mpf(&cmpf);
        if (ret > 0) { id_ = ret; return E_OK; }
        return static_cast<err_t>(ret);
    }

    void destroy() {
        if (id_ > 0) { tk_del_mpf(id_); id_ = 0; }
    }

    err_t get(void** blk, TMO tmout = TMO_FEVR) {
        return tk_get_mpf(id_, blk, tmout);
    }

    err_t release(void* blk) {
        return tk_rel_mpf(id_, blk);
    }

    err_t ref(T_RMPF& rmpf) const { return tk_ref_mpf(id_, &rmpf); }

    ID id() const { return id_; }
    bool valid() const { return id_ > 0; }

private:
    ID id_ = 0;
};

class VariableMemPool : non_copyable {
public:
    struct Config {
        void*   exinf   = nullptr;
        ATR     attr    = TA_TFIFO;
        SZ      poolsz  = 1024;
    };

    VariableMemPool() = default;

    explicit VariableMemPool(const Config& cfg) { create(cfg); }

    ~VariableMemPool() { destroy(); }

    VariableMemPool(VariableMemPool&& o) : id_(o.id_) { o.id_ = 0; }
    VariableMemPool& operator=(VariableMemPool&& o) {
        if (this != &o) { destroy(); id_ = o.id_; o.id_ = 0; }
        return *this;
    }

    err_t create(const Config& cfg) {
        T_CMPL cmpl = {};
        cmpl.exinf   = cfg.exinf;
        cmpl.mplatr  = cfg.attr;
        cmpl.mplsz   = cfg.poolsz;
        ID ret = tk_cre_mpl(&cmpl);
        if (ret > 0) { id_ = ret; return E_OK; }
        return static_cast<err_t>(ret);
    }

    void destroy() {
        if (id_ > 0) { tk_del_mpl(id_); id_ = 0; }
    }

    err_t get(SZ blksz, void** blk, TMO tmout = TMO_FEVR) {
        return tk_get_mpl(id_, blksz, blk, tmout);
    }

    err_t release(void* blk) {
        return tk_rel_mpl(id_, blk);
    }

    err_t ref(T_RMPL& rmpl) const { return tk_ref_mpl(id_, &rmpl); }

    ID id() const { return id_; }
    bool valid() const { return id_ > 0; }

private:
    ID id_ = 0;
};

} // namespace utk
