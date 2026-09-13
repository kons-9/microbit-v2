#pragma once

#include <tk/tkernel.h>
#include <cassert>

namespace utkernel {

inline event_flag::event_flag(uint32_t initial) {
    static_assert(sizeof(m_storage) >= sizeof(ID), "storage too small");
    T_CFLG cflg = {};
    cflg.exinf = nullptr;
    cflg.flgatr = TA_TFIFO | TA_WMUL;
    cflg.iflgptn = static_cast<UINT>(initial);
    ID id = tk_cre_flg(&cflg);
    assert(id > 0);
    *reinterpret_cast<ID *>(m_storage) = id;
}

inline event_flag::~event_flag() {
    ID id = *reinterpret_cast<ID *>(m_storage);
    if (id > 0) {
        tk_del_flg(id);
    }
}

inline void event_flag::set(uint32_t bits) {
    ID id = *reinterpret_cast<ID *>(m_storage);
    ER rc = tk_set_flg(id, static_cast<UINT>(bits));
    assert(rc == E_OK);
    (void)rc;
}

inline void event_flag::clear(uint32_t bits) {
    ID id = *reinterpret_cast<ID *>(m_storage);
    ER rc = tk_clr_flg(id, ~static_cast<UINT>(bits));
    assert(rc == E_OK);
    (void)rc;
}

inline uint32_t event_flag::wait(uint32_t pattern, wait_mode mode, uint32_t timeout_ms) {
    ID id = *reinterpret_cast<ID *>(m_storage);
    UINT flg_mode = (mode == any) ? TWF_ORW : TWF_ANDW;
    UINT flgptn = 0;
    TMO tmo = (timeout_ms == UINT32_MAX) ? TMO_FEVR : static_cast<TMO>(timeout_ms);
    ER rc = tk_wai_flg(id, static_cast<UINT>(pattern), flg_mode, &flgptn, tmo);
    if (rc == E_OK) {
        return static_cast<uint32_t>(flgptn);
    }
    return 0;
}

inline uint32_t event_flag::get() const {
    ID id = *reinterpret_cast<const ID *>(m_storage);
    T_RFLG rflg;
    ER rc = tk_ref_flg(id, &rflg);
    if (rc == E_OK) {
        return static_cast<uint32_t>(rflg.flgptn);
    }
    return 0;
}

}  // namespace utkernel
