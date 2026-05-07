#pragma once

#include <tk/tkernel.h>
#include <cassert>

namespace osal {

template <int LeastMaxValue>
inline counting_semaphore<LeastMaxValue>::counting_semaphore(int initial) {
    static_assert(sizeof(storage_) >= sizeof(ID), "storage too small for semaphore ID");
    T_CSEM csem = {};
    csem.exinf = nullptr;
    csem.sematr = TA_TFIFO | TA_CNT;
    csem.isemcnt = static_cast<INT>(initial);
    csem.maxsem = static_cast<INT>(LeastMaxValue);
    ID id = tk_cre_sem(&csem);
    assert(id > 0);
    *reinterpret_cast<ID *>(storage_) = id;
}

template <int LeastMaxValue>
inline counting_semaphore<LeastMaxValue>::~counting_semaphore() {
    ID id = *reinterpret_cast<ID *>(storage_);
    if (id > 0) {
        tk_del_sem(id);
    }
}

template <int LeastMaxValue>
inline void counting_semaphore<LeastMaxValue>::acquire() {
    ID id = *reinterpret_cast<ID *>(storage_);
    ER rc = tk_wai_sem(id, 1, TMO_FEVR);
    assert(rc == E_OK);
    (void)rc;
}

template <int LeastMaxValue>
inline bool counting_semaphore<LeastMaxValue>::try_acquire() {
    ID id = *reinterpret_cast<ID *>(storage_);
    return tk_wai_sem(id, 1, TMO_POL) == E_OK;
}

template <int LeastMaxValue>
inline bool counting_semaphore<LeastMaxValue>::try_acquire_for(uint32_t timeout_ms) {
    ID id = *reinterpret_cast<ID *>(storage_);
    return tk_wai_sem(id, 1, static_cast<TMO>(timeout_ms)) == E_OK;
}

template <int LeastMaxValue>
inline void counting_semaphore<LeastMaxValue>::release(int update) {
    ID id = *reinterpret_cast<ID *>(storage_);
    ER rc = tk_sig_sem(id, static_cast<INT>(update));
    assert(rc == E_OK);
    (void)rc;
}

}  // namespace osal
