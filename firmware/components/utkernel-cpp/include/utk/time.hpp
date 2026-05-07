#pragma once

#include "base.hpp"

namespace utk {

// System time utilities
namespace time {

inline err_t get(SYSTIM& tim) { return tk_get_tim(&tim); }
inline err_t set(const SYSTIM& tim) { return tk_set_tim(&tim); }
inline err_t get_utc(SYSTIM& tim) { return tk_get_utc(&tim); }
inline err_t set_utc(const SYSTIM& tim) { return tk_set_utc(&tim); }
inline err_t get_otm(SYSTIM& tim) { return tk_get_otm(&tim); }

} // namespace time

// System info
namespace sys {

inline err_t ref(T_RSYS& rsys) { return tk_ref_sys(&rsys); }
inline err_t ref_ver(T_RVER& rver) { return tk_ref_ver(&rver); }
inline err_t dis_dispatch() { return tk_dis_dsp(); }
inline err_t ena_dispatch() { return tk_ena_dsp(); }
inline err_t rotate_rdq(PRI pri) { return tk_rot_rdq(pri); }

} // namespace sys

} // namespace utk
