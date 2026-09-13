#pragma once

#include <tk/tkernel.h>
#include <cassert>

namespace utkernel {

template <int LeastMaxValue>
inline counting_semaphore<LeastMaxValue>::counting_semaphore(int32_t initial) {
    static_assert(sizeof(m_storage) >= sizeof(ID), "storage too small");
    T_CSEM csem = {};
    csem.exinf = nullptr;
    csem.sematr = TA_TFIFO | TA_CNT;
    csem.isemcnt = static_cast<INT>(initial);
    csem.maxsem = static_cast<INT>(LeastMaxValue);
    ID id = tk_cre_sem(&csem);
    assert(id > 0);
    *reinterpret_cast<ID *>(m_storage) = id;
}

template <int LeastMaxValue>
inline counting_semaphore<LeastMaxValue>::~counting_semaphore() {
    ID id = *reinterpret_cast<ID *>(m_storage);
    if (id > 0) {
        tk_del_sem(id);
    }
}

template <int LeastMaxValue>
inline void counting_semaphore<LeastMaxValue>::acquire() {
    ID id = *reinterpret_cast<ID *>(m_storage);
    ER rc = tk_wai_sem(id, 1, TMO_FEVR);
    assert(rc == E_OK);
    (void)rc;
}

template <int LeastMaxValue>
inline bool counting_semaphore<LeastMaxValue>::try_acquire() {
    ID id = *reinterpret_cast<ID *>(m_storage);
    return tk_wai_sem(id, 1, TMO_POL) == E_OK;
}

template <int LeastMaxValue>
inline bool counting_semaphore<LeastMaxValue>::try_acquire_for(uint32_t timeout_ms) {
    ID id = *reinterpret_cast<ID *>(m_storage);
    return tk_wai_sem(id, 1, static_cast<TMO>(timeout_ms)) == E_OK;
}

template <int LeastMaxValue>
inline void counting_semaphore<LeastMaxValue>::release(int32_t update) {
    ID id = *reinterpret_cast<ID *>(m_storage);
    ER rc = tk_sig_sem(id, static_cast<INT>(update));
    assert(rc == E_OK);
    (void)rc;
}

}  // namespace utkernel
