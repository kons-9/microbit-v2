# kernel/cmake/sources_microbit.cmake
# μT-Kernel 3 micro:bit v2 (nRF52833 / Cortex-M4) 固有ソース

set(KERNEL_ARCH_SOURCES
    # lib/libtm sysdepend
    ${MTKERNEL_ROOT}/lib/libtm/sysdepend/microbit/tm_com.c
    ${MTKERNEL_ROOT}/lib/libtm/sysdepend/no_device/tm_com.c

    # lib/libtk sysdepend
    ${MTKERNEL_ROOT}/lib/libtk/sysdepend/cpu/nrf5/int_nrf5.c
    ${MTKERNEL_ROOT}/lib/libtk/sysdepend/cpu/nrf5/ptimer_nrf5.c
    ${MTKERNEL_ROOT}/lib/libtk/sysdepend/cpu/core/armv7m/int_armv7m.c
    ${MTKERNEL_ROOT}/lib/libtk/sysdepend/cpu/core/armv7m/wusec_armv7m.c

    # kernel/sysdepend board
    ${MTKERNEL_ROOT}/kernel/sysdepend/microbit/devinit.c
    ${MTKERNEL_ROOT}/kernel/sysdepend/microbit/hw_setting.c
    ${MTKERNEL_ROOT}/kernel/sysdepend/microbit/power_save.c

    # kernel/sysdepend cpu
    ${MTKERNEL_ROOT}/kernel/sysdepend/cpu/nrf5/cpu_clock.c
    ${MTKERNEL_ROOT}/kernel/sysdepend/cpu/nrf5/vector_tbl.c

    # kernel/sysdepend core (ARMv7-M)
    ${MTKERNEL_ROOT}/kernel/sysdepend/cpu/core/armv7m/cpu_cntl.c
    ${MTKERNEL_ROOT}/kernel/sysdepend/cpu/core/armv7m/dispatch.S
    ${MTKERNEL_ROOT}/kernel/sysdepend/cpu/core/armv7m/exc_hdr.c
    ${MTKERNEL_ROOT}/kernel/sysdepend/cpu/core/armv7m/interrupt.c
    ${MTKERNEL_ROOT}/kernel/sysdepend/cpu/core/armv7m/reset_hdl.c

    # device/ser sysdepend
    ${MTKERNEL_ROOT}/device/ser/sysdepend/nrf5/ser_nrf5.c
)

set(KERNEL_ARCH_COMPILE_DEFINITIONS
    _MICROBIT_
)
