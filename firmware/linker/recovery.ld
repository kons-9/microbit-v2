/*
 * recovery.ld
 * Linker script for the emergency log reader recovery mode
 *
 * Placed in the Recovery slot: 0x0006E000 - 0x00077FFF (40 KB)
 * SoftDevice is not used.
 */

INCLUDE "memory_map.ld"

ENTRY(Reset_Handler)

MEMORY
{
  ROM (rx)  : ORIGIN = __recovery_start, LENGTH = __recovery_end - __recovery_start
  RAM (xrw) : ORIGIN = __ram_start, LENGTH = __ram_size
}

SECTIONS {
    .text : {
        . = ALIGN(8);
        __vector_org = .;
        *(.vector)
    } >ROM

    .init : {
        KEEP(*(.init))
    } >ROM

    .text.app : {
        __start = .;
        *(.text)
        *(.text.*)
        *(.rodata)
        *(.rodata.*)

        /* recovery does not call __libc_init_array(); retaining C++ init
         * arrays pulls in the atexit/newlib runtime and exceeds this slot.
         */
    } >ROM

    .fini : {
        KEEP(*(.fini))
    } >ROM

    . = ALIGN(4);
    __data_org = .;

    vector __ram_start (NOLOAD) : {
        __vector_start = .;
        *(.data_vector)
        __vector_end = .;
    } >RAM

    .data : AT(__data_org) {
        __data_start = .;
        *(.data)
        *(.data.*)
        . = ALIGN(4);
        __data_end = .;
    } >RAM

    .bss ALIGN(4) (NOLOAD) : {
        __bss_start = .;
        PROVIDE (__noinit_start = .);
        *(.noinit)
        PROVIDE (__noinit_end = .);
        *(.bss)
        *(.bss.*)
        *(COMMON)
        . = ALIGN(4);
        __bss_end = .;
        __end = .;
        PROVIDE(end = .);
    } >RAM
}
