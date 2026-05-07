#pragma once

#include "base.hpp"

namespace utk {

class MessageBuffer : non_copyable {
public:
    struct Config {
        void*   exinf   = nullptr;
        ATR     attr    = TA_TFIFO;
        SZ      bufsz   = 256;
        INT     maxmsz  = 64;
    };

    MessageBuffer() = default;

    explicit MessageBuffer(const Config& cfg) { create(cfg); }

    ~MessageBuffer() { destroy(); }

    MessageBuffer(MessageBuffer&& o) : id_(o.id_) { o.id_ = 0; }
    MessageBuffer& operator=(MessageBuffer&& o) {
        if (this != &o) { destroy(); id_ = o.id_; o.id_ = 0; }
        return *this;
    }

    err_t create(const Config& cfg) {
        T_CMBF cmbf = {};
        cmbf.exinf   = cfg.exinf;
        cmbf.mbfatr  = cfg.attr;
        cmbf.bufsz   = cfg.bufsz;
        cmbf.maxmsz  = cfg.maxmsz;
        ID ret = tk_cre_mbf(&cmbf);
        if (ret > 0) { id_ = ret; return E_OK; }
        return static_cast<err_t>(ret);
    }

    void destroy() {
        if (id_ > 0) { tk_del_mbf(id_); id_ = 0; }
    }

    err_t send(const void* msg, INT msgsz, TMO tmout = TMO_FEVR) {
        return tk_snd_mbf(id_, msg, msgsz, tmout);
    }

    INT receive(void* msg, TMO tmout = TMO_FEVR) {
        return tk_rcv_mbf(id_, msg, tmout);
    }

    err_t ref(T_RMBF& rmbf) const { return tk_ref_mbf(id_, &rmbf); }

    ID id() const { return id_; }
    bool valid() const { return id_ > 0; }

private:
    ID id_ = 0;
};

} // namespace utk
