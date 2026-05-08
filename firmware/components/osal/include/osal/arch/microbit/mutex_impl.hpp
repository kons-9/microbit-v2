#pragma once

#include <tk/tkernel.h>
#include <cassert>

namespace osal {

inline mutex::mutex() {
    static_assert(sizeof(m_storage) >= sizeof(ID), "storage too small");
    T_CMTX cmtx = {};
    cmtx.exinf = nullptr;
    cmtx.mtxatr = TA_INHERIT;
    cmtx.ceilpri = 0;
    ID id = tk_cre_mtx(&cmtx);
    assert(id > 0);
    *reinterpret_cast<ID *>(m_storage) = id;
}

inline mutex::~mutex() {
    ID id = *reinterpret_cast<ID *>(m_storage);
    if (id > 0) {
        tk_del_mtx(id);
    }
}

inline void mutex::lock() {
    ID id = *reinterpret_cast<ID *>(m_storage);
    ER rc = tk_loc_mtx(id, TMO_FEVR);
    assert(rc == E_OK);
    (void)rc;
}

inline bool mutex::try_lock() {
    ID id = *reinterpret_cast<ID *>(m_storage);
    return tk_loc_mtx(id, TMO_POL) == E_OK;
}

inline void mutex::unlock() {
    ID id = *reinterpret_cast<ID *>(m_storage);
    ER rc = tk_unl_mtx(id);
    assert(rc == E_OK);
    (void)rc;
}

}  // namespace osal
