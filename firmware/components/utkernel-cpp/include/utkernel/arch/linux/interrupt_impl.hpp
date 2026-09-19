#pragma once

namespace utkernel {

inline bool interrupt::define(uint32_t intno, handler_t handler, bool high_level) {
    (void)intno;
    (void)handler;
    (void)high_level;
    return false;
}

inline void interrupt::reset() {
    disable();
    m_defined = false;
}

inline bool interrupt::enable(uint32_t priority) {
    (void)priority;
    return false;
}

inline void interrupt::disable() {
    m_enabled = false;
}

}  // namespace utkernel
