/*
 * memory_map.ld
 * nRF52833 (micro:bit v2) OTA memory layout - shared definitions
 *
 * このファイルを各リンカスクリプトから INCLUDE して使う。
 * メモリレイアウトの一元管理。
 */

/* ---- Flash Layout (512 KB total) ---- */
__softdevice_end   = 0x00026000;  /* SoftDevice S140: 0x00000000 - 0x00025FFF (152 KB) */
__app_slot_start   = 0x00026000;  /* Application slot start */
__app_slot_end     = 0x0006E000;  /* Application slot end (288 KB) */
__updater_start    = 0x0006E000;  /* Updater start */
__updater_end      = 0x00078000;  /* Updater end (40 KB) */
__bootloader_start = 0x00078000;  /* Bootloader start */
__bootloader_end   = 0x0007F000;  /* Bootloader end (28 KB) */
__settings_start   = 0x0007F000;  /* Settings page (4 KB) */
__flash_end        = 0x00080000;  /* End of flash */

/* ---- RAM Layout (128 KB total) ---- */
__ram_start        = 0x20000000;
__ram_size         = 128K;
__softdevice_ram   = 0x20002000;  /* SoftDevice RAM reservation (8 KB) */
