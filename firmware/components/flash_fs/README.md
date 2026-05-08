# Flash FS コンポーネント

NOR Flash 上の軽量ファイルシステム。Stream モード（追記ログ）と Block モード（固定位置 R/W）を提供。

## 設計

- **NOR Flash 特性を考慮**: ページ単位消去 (4KB)、書き込みは 0→1 不可
- **write_offset をヘッダに保存しない**: NOR Flash の AND セマンティクスで増加値を書けないため、起動時にページをスキャンして復元
- **ページヘッダ**: 4 bytes (sequence number のみ)
- **ページローテーション**: Stream ファイルは複数ページを循環使用

## ファイル一覧

| ID | 名前 | モード | ページ数 | 用途 |
|---|---|---|---|---|
| `FLASH_FS_FILE_LOG` | log | Stream | 4 | バイナリログ (flash_log) |
| `FLASH_FS_FILE_CONFIG` | config | Block | 1 | 設定データ |
| `FLASH_FS_FILE_OTA` | ota_staging | Stream | 3 | OTA 受信バッファ |

## API

- `flash_fs_init()` - 初期化（全ファイルのページスキャン）
- `flash_fs_append(file, data, len)` - Stream 追記
- `flash_fs_read(file, offset, buf, len)` - 読み出し
- `flash_fs_block_write(file, offset, data, len)` - Block 書き込み
- `flash_fs_block_read(file, offset, buf, len)` - Block 読み出し
- `flash_fs_erase(file)` - ファイル消去
- `flash_fs_get_info(file)` - 容量・使用量取得
- `flash_fs_get_name(file)` - ファイル名取得
- `flash_fs_find_by_name(name)` - 名前からファイルID検索

## テスト

```bash
cd firmware && make test-flash_fs   # 13 件
```
