/*
 * memory_map.ld
 * nRF52833 (micro:bit v2) application/recovery memory layout - shared definitions
 *
 * このファイルを各リンカスクリプトから INCLUDE して使う。
 * メモリレイアウトの一元管理。
 */

/* ---- Flash Layout (512 KB total, no SoftDevice) ---- */
__app_slot_start   = 0x00000000;  /* Application slot start */
__app_slot_end     = 0x00068000;  /* Application slot end (416 KB) */
__recovery_start   = 0x00068000;  /* Recovery start */
__recovery_end     = 0x00078000;  /* Recovery end (64 KB slot) */
__bootloader_start = 0x0007E000;  /* Reserved area start */
__bootloader_end   = 0x0007F000;  /* Reserved area end */
__settings_start   = 0x0007F000;  /* Settings page (4 KB) */
__flash_end        = 0x00080000;  /* End of flash */

/* ---- Flash FS regions ---- */
__fs_log_start      = 0x00078000;  /* Flash FS log (16 KB, 4 pages) */
__fs_settings_start = 0x0007C000;  /* Flash FS settings (4 KB, 1 page) */
__fs_calib_start    = 0x0007D000;  /* Flash FS calibration (4 KB, 1 page) */

/* ---- RAM Layout (128 KB total) ---- */
__ram_start        = 0x20000000;
__ram_size         = 128K;
