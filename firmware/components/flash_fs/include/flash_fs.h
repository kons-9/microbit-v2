#pragma once

/**
 * @file flash_fs.h
 * @brief 軽量 Flash ファイルシステム
 *
 * enum ベースの固定ファイルID で管理する最小ファイルシステム。
 *
 * ファイルモード:
 *   - Stream: append-only ring buffer (ログ向け)
 *   - Block:  固定領域 read/write (設定向け)
 *
 * Flash ページ割当はコンパイル時テーブルで決定する。
 */

#include <cstdint>
#include <cstddef>

/* ================================================================== */
/*  File ID                                                           */
/* ================================================================== */

enum FlashFsFileId {
    FLASH_FS_FILE_LOG = 0,
    FLASH_FS_FILE_SETTINGS = 1,
    FLASH_FS_FILE_CALIB = 2,
    FLASH_FS_FILE_COUNT,
};

/* ================================================================== */
/*  File Mode                                                         */
/* ================================================================== */

enum FlashFsMode {
    FLASH_FS_MODE_STREAM = 0, /**< Append-only ring buffer */
    FLASH_FS_MODE_BLOCK = 1,  /**< Fixed-block read/write */
};

/* ================================================================== */
/*  File Info (query)                                                  */
/* ================================================================== */

struct FlashFsFileInfo {
    FlashFsFileId id;
    FlashFsMode mode;
    const char *name;
    uint32_t capacity; /**< Total usable bytes */
    uint32_t used;     /**< Currently used bytes */
};

/* ================================================================== */
/*  Init                                                              */
/* ================================================================== */

/**
 * @brief Flash FS を初期化する
 * @return 0: 成功, 負: エラー
 *
 * 内部でページヘッダをスキャンし、stream の write position を復元する。
 */
int32_t flash_fs_init(void);

/* ================================================================== */
/*  Name ↔ ID 変換                                                    */
/* ================================================================== */

/**
 * @brief FileId → 文字列名
 */
const char *flash_fs_get_name(FlashFsFileId id);

/**
 * @brief 文字列名 → FileId
 * @return 見つからなければ FLASH_FS_FILE_COUNT
 */
FlashFsFileId flash_fs_find_by_name(const char *name);

/* ================================================================== */
/*  Stream API (Log 向け)                                             */
/* ================================================================== */

/**
 * @brief Stream ファイルにデータを追記する
 * @param id    ファイルID (mode == STREAM であること)
 * @param data  書き込みデータ
 * @param size  バイト数
 * @return true: 成功
 *
 * 現在ページに入りきらない場合は次ページへ進む (ringで最古ページをerase)。
 */
bool flash_fs_append(FlashFsFileId id, const void *data, size_t size);

/**
 * @brief Stream ファイルを先頭から読み出す
 * @param id      ファイルID
 * @param offset  先頭からのバイトオフセット
 * @param buf     読み出しバッファ
 * @param size    読み出しバイト数
 * @return 実際に読めたバイト数
 */
size_t flash_fs_read(FlashFsFileId id, uint32_t offset, void *buf, size_t size);

/* ================================================================== */
/*  Block API (Settings 向け)                                         */
/* ================================================================== */

/**
 * @brief Block ファイルにデータを書き込む
 * @param id      ファイルID (mode == BLOCK であること)
 * @param offset  ファイル内オフセット
 * @param data    書き込みデータ
 * @param size    バイト数
 * @return true: 成功
 */
bool flash_fs_block_write(FlashFsFileId id, uint32_t offset, const void *data, size_t size);

/**
 * @brief Block ファイルからデータを読み出す
 */
size_t flash_fs_block_read(FlashFsFileId id, uint32_t offset, void *buf, size_t size);

/* ================================================================== */
/*  共通操作                                                           */
/* ================================================================== */

/**
 * @brief ファイルを消去する
 */
void flash_fs_erase(FlashFsFileId id);

/**
 * @brief ファイル情報を取得する
 */
bool flash_fs_get_info(FlashFsFileId id, FlashFsFileInfo *info);
