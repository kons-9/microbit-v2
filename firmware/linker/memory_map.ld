/*
 * memory_map.ld
 * nRF52833 (micro:bit v2) OTA memory layout - shared definitions
 *
 * このファイルを各リンカスクリプトから INCLUDE して使う。
 * メモリレイアウトの一元管理。
 */

/* ---- Flash Layout (512 KB total) ---- */
__softdevice_end   = 0x00027000;  /* SoftDevice S140 v7.3.0: hex extends to 0x26498, next page = 0x27000 */
__app_slot_start   = 0x00027000;  /* Application slot start (after SoftDevice page) */
__app_slot_end     = 0x0006E000;  /* Application slot end (284 KB) */
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
__softdevice_ram   = 0x20002000;  /* SoftDevice RAM reservation (8 KB) */
