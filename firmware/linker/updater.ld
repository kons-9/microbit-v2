/*
 * updater.ld
 * Linker script for the OTA updater
 *
 * Placed in the Updater slot: 0x0006E000 - 0x00077FFF (40 KB)
 * SoftDevice is not used.
 */

INCLUDE "memory_map.ld"

ENTRY(Reset_Handler)

MEMORY
{
  ROM (rx)  : ORIGIN = __updater_start, LENGTH = __updater_end - __updater_start
  RAM (xrw) : ORIGIN = __ram_start, LENGTH = __ram_size
}

SECTIONS {
    .text : {
        . = ALIGN(8);
        __vector_org = .;
        *(.vector)
        __start = .;
        *(.text)
        *(.text.*)
        *(.rodata)
        *(.rodata.*)
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
    } >RAM
}
