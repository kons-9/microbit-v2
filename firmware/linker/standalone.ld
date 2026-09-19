/*
 * standalone.ld
 * Linker script for standalone operation (without SoftDevice)
 *
 * ROM starts at 0x00000000, uses full flash and RAM.
 * Use this for testing without SoftDevice on nRF52833 (micro:bit v2).
 */

ENTRY(Reset_Handler)

MEMORY
{
  ROM (rx)  : ORIGIN = 0x00000000, LENGTH = 512K
  RAM (xrw) : ORIGIN = 0x20000000, LENGTH = 128K
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

        . = ALIGN(4);
        PROVIDE_HIDDEN(__preinit_array_start = .);
        KEEP(*(.preinit_array))
        PROVIDE_HIDDEN(__preinit_array_end = .);

        . = ALIGN(4);
        PROVIDE_HIDDEN(__init_array_start = .);
        KEEP(*(SORT(.init_array.*)))
        KEEP(*(.init_array))
        PROVIDE_HIDDEN(__init_array_end = .);

        . = ALIGN(4);
        PROVIDE_HIDDEN(__fini_array_start = .);
        KEEP(*(SORT(.fini_array.*)))
        KEEP(*(.fini_array))
        PROVIDE_HIDDEN(__fini_array_end = .);
    } >ROM

    .fini : {
        KEEP(*(.fini))
    } >ROM

    . = ALIGN(4);
    __data_org = .;

    vector 0x20000000 (NOLOAD) : {
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
