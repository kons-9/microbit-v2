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
#include <option.h>
#include <result.h>

namespace flash_fs {

enum class Error : uint8_t {
    InvalidFile,
    InvalidArgument,
    WrongMode,
    OutOfBounds,
    TooLarge,
};

/* ================================================================== */
/*  File ID                                                           */
/* ================================================================== */

enum FileId {
    FILE_LOG = 0,
    FILE_SETTINGS = 1,
    FILE_CALIB = 2,
    FILE_COUNT,
};

/* ================================================================== */
/*  File Mode                                                         */
/* ================================================================== */

enum Mode {
    MODE_STREAM = 0, /**< Append-only ring buffer */
    MODE_BLOCK = 1,  /**< Fixed-block read/write */
};

/* ================================================================== */
/*  File Info (query)                                                  */
/* ================================================================== */

struct FileInfo {
    FileId id;
    Mode mode;
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
int32_t init(void);

/* ================================================================== */
/*  Name ↔ ID 変換                                                    */
/* ================================================================== */

/**
 * @brief FileId → 文字列名
 */
library::Option<const char *> get_name(FileId id);

/**
 * @brief 文字列名 → FileId
 * @return 見つからなければ FILE_COUNT
 */
library::Option<FileId> find_by_name(const char *name);

/* ================================================================== */
/*  Stream API (Log 向け)                                             */
/* ================================================================== */

/**
 * @brief Stream ファイルにデータを追記する
 * @param id    ファイルID (mode == STREAM であること)
 * @param data  書き込みデータ
 * @param size  バイト数
 * @return 成功、または InvalidFile / WrongMode / TooLarge
 *
 * 現在ページに入りきらない場合は次ページへ進む (ringで最古ページをerase)。
 */
library::Result<void, Error> append(FileId id, const void *data, size_t size);

/**
 * @brief Stream ファイルを先頭から読み出す
 * @param id      ファイルID
 * @param offset  先頭からのバイトオフセット
 * @param buf     読み出しバッファ
 * @param size    読み出しバイト数
 * @return 実際に読めたバイト数
 */
size_t read(FileId id, uint32_t offset, void *buf, size_t size);

/* ================================================================== */
/*  Block API (Settings 向け)                                         */
/* ================================================================== */

/**
 * @brief Block ファイルにデータを書き込む
 * @param id      ファイルID (mode == BLOCK であること)
 * @param offset  ファイル内オフセット
 * @param data    書き込みデータ
 * @param size    バイト数
 * @return 成功、または InvalidFile / WrongMode / OutOfBounds
 */
library::Result<void, Error> block_write(FileId id, uint32_t offset, const void *data, size_t size);

/**
 * @brief Block ファイルからデータを読み出す
 */
size_t block_read(FileId id, uint32_t offset, void *buf, size_t size);

/* ================================================================== */
/*  共通操作                                                           */
/* ================================================================== */

/**
 * @brief ファイルを消去する
 */
void erase(FileId id);

/**
 * @brief ファイル情報を取得する
 * @return 成功、または InvalidFile / InvalidArgument
 */
library::Result<void, Error> get_info(FileId id, FileInfo *info);

}  // namespace flash_fs
