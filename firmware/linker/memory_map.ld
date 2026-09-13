/*
 * memory_map.ld
 * nRF52833 (micro:bit v2) OTA memory layout - shared definitions
 *
 * このファイルを各リンカスクリプトから INCLUDE して使う。
 * メモリレイアウトの一元管理。
 */

/* ---- Flash Layout (512 KB total, no SoftDevice) ---- */
__app_slot_start   = 0x00000000;  /* Application slot start */
__app_slot_end     = 0x0006E000;  /* Application slot end (440 KB) */
__updater_start    = 0x0006E000;  /* Updater start */
__updater_end      = 0x00078000;  /* Updater end (40 KB) */
__bootloader_start = 0x00078000;  /* Bootloader start */
__bootloader_end   = 0x0007F000;  /* Bootloader end (28 KB) */
__settings_start   = 0x0007F000;  /* Settings page (4 KB) */
__flash_end        = 0x00080000;  /* End of flash */

/* ---- Flash FS regions ---- */
__flash_fs_log_start      = 0x00070000;  /* Flash FS log (16 KB, 4 pages) */
__flash_fs_settings_start = 0x00074000;  /* Flash FS settings (4 KB, 1 page) */
__flash_fs_calib_start    = 0x00075000;  /* Flash FS calibration / OTA staging (12 KB, 3 pages) */

/* ---- RAM Layout (128 KB total) ---- */
__ram_start        = 0x20000000;
__ram_size         = 128K;
