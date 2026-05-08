# kernel/cmake/sources_common.cmake
# μT-Kernel 3 共通ソース (アーキテクチャ非依存)

set(KERNEL_COMMON_SOURCES
    # lib/libtm
    ${MTKERNEL_ROOT}/lib/libtm/libtm.c
    ${MTKERNEL_ROOT}/lib/libtm/libtm_printf.c

    # lib/libtk
    ${MTKERNEL_ROOT}/lib/libtk/fastlock.c
    ${MTKERNEL_ROOT}/lib/libtk/fastmlock.c
    ${MTKERNEL_ROOT}/lib/libtk/kmalloc.c

    # kernel/tstdlib
    ${MTKERNEL_ROOT}/kernel/tstdlib/bitop.c
    ${MTKERNEL_ROOT}/kernel/tstdlib/string.c

    # kernel/tkernel
    ${MTKERNEL_ROOT}/kernel/tkernel/cpuctl.c
    ${MTKERNEL_ROOT}/kernel/tkernel/device.c
    ${MTKERNEL_ROOT}/kernel/tkernel/deviceio.c
    ${MTKERNEL_ROOT}/kernel/tkernel/eventflag.c
    ${MTKERNEL_ROOT}/kernel/tkernel/int.c
    ${MTKERNEL_ROOT}/kernel/tkernel/klock.c
    ${MTKERNEL_ROOT}/kernel/tkernel/mailbox.c
    ${MTKERNEL_ROOT}/kernel/tkernel/memory.c
    ${MTKERNEL_ROOT}/kernel/tkernel/mempfix.c
    ${MTKERNEL_ROOT}/kernel/tkernel/mempool.c
    ${MTKERNEL_ROOT}/kernel/tkernel/messagebuf.c
    ${MTKERNEL_ROOT}/kernel/tkernel/misc_calls.c
    ${MTKERNEL_ROOT}/kernel/tkernel/mutex.c
    ${MTKERNEL_ROOT}/kernel/tkernel/objname.c
    ${MTKERNEL_ROOT}/kernel/tkernel/power.c
    ${MTKERNEL_ROOT}/kernel/tkernel/rendezvous.c
    ${MTKERNEL_ROOT}/kernel/tkernel/semaphore.c
    ${MTKERNEL_ROOT}/kernel/tkernel/task.c
    ${MTKERNEL_ROOT}/kernel/tkernel/task_manage.c
    ${MTKERNEL_ROOT}/kernel/tkernel/task_sync.c
    ${MTKERNEL_ROOT}/kernel/tkernel/time_calls.c
    ${MTKERNEL_ROOT}/kernel/tkernel/timer.c
    ${MTKERNEL_ROOT}/kernel/tkernel/tkinit.c
    ${MTKERNEL_ROOT}/kernel/tkernel/wait.c

    # kernel/sysinit
    ${MTKERNEL_ROOT}/kernel/sysinit/sysinit.c

    # kernel/inittask
    ${MTKERNEL_ROOT}/kernel/inittask/inittask.c

    # device/common
    ${MTKERNEL_ROOT}/device/common/drvif/msdrvif.c

    # device drivers (common parts)
    ${MTKERNEL_ROOT}/device/adc/adc.c
    ${MTKERNEL_ROOT}/device/i2c/i2c.c
    ${MTKERNEL_ROOT}/device/ser/ser.c
)
