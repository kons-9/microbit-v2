#pragma once

#include <tk/tkernel.h>
#include <cassert>

namespace osal {

inline mutex::mutex() {
    static_assert(sizeof(storage_) >= sizeof(ID), "storage too small for mutex ID");
    T_CMTX cmtx = {};
    cmtx.exinf = nullptr;
    cmtx.mtxatr = TA_INHERIT;
    cmtx.ceilpri = 0;
    ID id = tk_cre_mtx(&cmtx);
    assert(id > 0);
    *reinterpret_cast<ID *>(storage_) = id;
}

inline mutex::~mutex() {
    ID id = *reinterpret_cast<ID *>(storage_);
    if (id > 0) {
        tk_del_mtx(id);
    }
}

inline void mutex::lock() {
    ID id = *reinterpret_cast<ID *>(storage_);
    ER rc = tk_loc_mtx(id, TMO_FEVR);
    assert(rc == E_OK);
    (void)rc;
}

inline bool mutex::try_lock() {
    ID id = *reinterpret_cast<ID *>(storage_);
    return tk_loc_mtx(id, TMO_POL) == E_OK;
}

inline void mutex::unlock() {
    ID id = *reinterpret_cast<ID *>(storage_);
    ER rc = tk_unl_mtx(id);
    assert(rc == E_OK);
    (void)rc;
}

}  // namespace osal
