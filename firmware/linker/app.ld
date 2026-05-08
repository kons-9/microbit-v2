/*
 * app.ld
 * Linker script for the main application (ble-locator)
 *
 * Placed in the Application slot: 0x00026000 - 0x0006DFFF (288 KB)
 * RAM: after SoftDevice reservation
 */

INCLUDE "memory_map.ld"

ENTRY(Reset_Handler)

MEMORY
{
  ROM (rx)  : ORIGIN = __app_slot_start, LENGTH = __app_slot_end - __app_slot_start
  RAM (xrw) : ORIGIN = __softdevice_ram, LENGTH = __ram_start + __ram_size - __softdevice_ram
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

    vector __softdevice_ram (NOLOAD) : {
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
