/*
 * nrfx_glue.h — μT-Kernel 用 nrfx ポーティングレイヤ
 *
 * CMSIS 関数をそのまま使用する最小実装。
 */

#ifndef NRFX_GLUE_H__
#define NRFX_GLUE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "nrf.h"

/*--- Atomic type ---*/
typedef volatile uint32_t nrfx_atomic_t;

/*--- Assertions ---*/
#define NRFX_ASSERT(expression)         do { (void)(expression); } while (0)
#define NRFX_STATIC_ASSERT(expression)  _Static_assert(expression, "nrfx static assert")

/*--- IRQ ---*/
#define NRFX_IRQ_PRIORITY_SET(irq_number, priority) \
    NVIC_SetPriority((IRQn_Type)(irq_number), (priority))

#define NRFX_IRQ_ENABLE(irq_number)     NVIC_EnableIRQ((IRQn_Type)(irq_number))
#define NRFX_IRQ_IS_ENABLED(irq_number) NVIC_GetEnableIRQ((IRQn_Type)(irq_number))
#define NRFX_IRQ_DISABLE(irq_number)    NVIC_DisableIRQ((IRQn_Type)(irq_number))
#define NRFX_IRQ_PENDING_SET(irq_number)    NVIC_SetPendingIRQ((IRQn_Type)(irq_number))
#define NRFX_IRQ_PENDING_CLEAR(irq_number)  NVIC_ClearPendingIRQ((IRQn_Type)(irq_number))
#define NRFX_IRQ_IS_PENDING(irq_number)     NVIC_GetPendingIRQ((IRQn_Type)(irq_number))

/*--- Critical section ---*/
#define NRFX_CRITICAL_SECTION_ENTER()   __disable_irq()
#define NRFX_CRITICAL_SECTION_EXIT()    __enable_irq()

/*--- Delay ---*/
#define NRFX_COREDEP_DELAY_DWT_BASED    0

#include "nrfx_coredep.h"
#define NRFX_DELAY_US(us_time)  nrfx_coredep_delay_us(us_time)

/*--- Memory barrier ---*/
#define NRFX_ATOMIC_FETCH_OR(p_data, value)   nrfx_atomic_u32_fetch_or(p_data, value)
#define NRFX_ATOMIC_FETCH_AND(p_data, value)  nrfx_atomic_u32_fetch_and(p_data, value)
#define NRFX_ATOMIC_FETCH_XOR(p_data, value)  nrfx_atomic_u32_fetch_xor(p_data, value)
#define NRFX_ATOMIC_FETCH_STORE(p_data, value) nrfx_atomic_u32_fetch_store(p_data, value)

/*--- Bit counting (CLZ / CTZ) ---*/
#define NRFX_CLZ(value)  __CLZ(value)
#define NRFX_CTZ(value)  __CLZ(__RBIT(value))

/*--- Event / PPI (unused, stubs) ---*/
#define NRFX_EVENT_READBACK_ENABLED  0

#ifdef __cplusplus
}
#endif

#endif /* NRFX_GLUE_H__ */
