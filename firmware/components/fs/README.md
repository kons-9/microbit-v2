# Flash FS コンポーネント

NOR Flash 上の軽量ファイルシステム。RingBuffer file（追記ログ）と Fixed file（固定位置 R/W）を提供する。

## 設計

- **NOR Flash 特性を考慮**: ページ単位消去 (4KB)、書き込みは 0→1 不可
- **write_offset をヘッダに保存しない**: NOR Flash の AND セマンティクスで増加値を書けないため、起動時にページをスキャンして復元
- **ページヘッダ**: 4 bytes (sequence number のみ)
- **ページローテーション**: RingBuffer file は複数ページを循環使用

## ファイル一覧

| ID | 名前 | 種類 | ページ数 | 用途 |
|---|---|---|---|---|
| `FileId::Log` | log | RingBuffer | 4 | ログデータ |
| `FileId::Settings` | settings | Fixed | 1 | 設定データ |
| `FileId::Calib` | calib | Fixed | 1 | キャリブレーションデータ |

## API

`FileSystem` は Flash ドライバをコンストラクタから受け取る。グローバルな
ファイルシステムは持たず、アプリケーションの `Drivers`／`Config` から
インスタンスを生成する。

```cpp
#include "flash.h"

struct Drivers {
    drivers::Flash flash;
};

struct Config {
    fs::FileSystem file_system;

    explicit Config(Drivers &drivers) : file_system(drivers.flash) {}
};

Drivers drivers;
Config config(drivers);

config.file_system.init();
config.file_system.append(fs::FileId::Log, data, size);
```

主なメソッドは `init`、`append`、`read`、`block_write`、`block_read`、
`erase`、`get_info`、`get_name`、`find_by_name`。

`RingBufferFile`と`FixedFile`は`io::Stream`を実装するため、Loggerなどの
出力先へそのまま注入できる。

## テスト

```bash
cd firmware && make test-fs   # 13 件
```
