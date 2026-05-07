#pragma once

#include "base.hpp"

namespace utk {

class Task : non_copyable {
public:
    using func_t = void (*)(INT stacd, void* exinf);

    struct Config {
        void*   exinf   = nullptr;
        ATR     attr    = TA_HLNG | TA_RNG3;
        func_t  task    = nullptr;
        PRI     pri     = 10;
        SZ      stksz   = 1024;
    };

    Task() = default;

    explicit Task(const Config& cfg) { create(cfg); }

    ~Task() { destroy(); }

    Task(Task&& o) : id_(o.id_) { o.id_ = 0; }
    Task& operator=(Task&& o) {
        if (this != &o) { destroy(); id_ = o.id_; o.id_ = 0; }
        return *this;
    }

    err_t create(const Config& cfg) {
        T_CTSK ctsk = {};
        ctsk.exinf   = cfg.exinf;
        ctsk.tskatr  = cfg.attr;
        ctsk.task    = reinterpret_cast<FP>(cfg.task);
        ctsk.itskpri = cfg.pri;
        ctsk.stksz   = cfg.stksz;
        ID ret = tk_cre_tsk(&ctsk);
        if (ret > 0) { id_ = ret; return E_OK; }
        return static_cast<err_t>(ret);
    }

    void destroy() {
        if (id_ > 0) { tk_del_tsk(id_); id_ = 0; }
    }

    err_t start(INT stacd = 0) { return tk_sta_tsk(id_, stacd); }
    err_t terminate()          { return tk_ter_tsk(id_); }
    err_t suspend()            { return tk_sus_tsk(id_); }
    err_t resume()             { return tk_rsm_tsk(id_); }
    err_t wakeup()             { return tk_wup_tsk(id_); }
    err_t release_wait()       { return tk_rel_wai(id_); }
    err_t change_pri(PRI pri)  { return tk_chg_pri(id_, pri); }

    err_t ref(T_RTSK& rtsk) const { return tk_ref_tsk(id_, &rtsk); }

    ID id() const { return id_; }
    bool valid() const { return id_ > 0; }

    // Static utilities
    static void exit()     { tk_ext_tsk(); }
    static void exit_del() { tk_exd_tsk(); }
    static err_t sleep(TMO tmout = TMO_FEVR) { return tk_slp_tsk(tmout); }
    static err_t delay(RELTIM ms) { return tk_dly_tsk(ms); }
    static ID    self()   { return tk_get_tid(); }

private:
    ID id_ = 0;
};

} // namespace utk
