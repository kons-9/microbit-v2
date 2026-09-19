#pragma once

#include <tk/tkernel.h>
#include <tk/syslib.h>

namespace utkernel {

inline bool interrupt::define(uint32_t intno, handler_t handler, bool high_level) {
    T_DINT dint = {};
    dint.intatr = high_level ? TA_HLNG : TA_ASM;
    dint.inthdr = reinterpret_cast<FP>(handler);
    if (tk_def_int(static_cast<UINT>(intno), &dint) < E_OK) {
        return false;
    }
    m_intno = intno;
    m_defined = true;
    return true;
}

inline bool interrupt::enable(uint32_t priority) {
    if (!m_defined) {
        return false;
    }
    EnableInt(static_cast<UINT>(m_intno), static_cast<UINT>(priority));
    m_enabled = true;
    return true;
}

inline void interrupt::disable() {
    if (m_enabled) {
        DisableInt(static_cast<UINT>(m_intno));
        m_enabled = false;
    }
}

inline void interrupt::reset() {
    if (!m_defined) {
        return;
    }
    disable();
    tk_def_int(static_cast<UINT>(m_intno), nullptr);
    m_defined = false;
}

}  // namespace utkernel
