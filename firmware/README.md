# firmware

micro:bit v2.2 (nRF52833) 向けファームウェア。μT-Kernel 3 RTOS 上で動作する BLE ロケータ。

## 前提条件

- `gcc-arm-none-eabi` (GNU Arm Embedded Toolchain)
- CMake 3.16+
- Python 3 + venv（書き込みに pyocd を使用）
- clang-format-18（コード整形）

## セットアップ

```bash
git submodule update --init --recursive

# μT-Kernel 3 のダウンロード・展開
# パスワード: https://www.t-engine4u.com/info/mbit/2.html
cd kernel && ./setup.sh && cd ..
```

## コマンド一覧

| コマンド | 説明 |
|---|---|
| `make build` | メインアプリをビルド |
| `make build-updater` | OTA アップデータをビルド |
| `make flash` | メインアプリを書き込み |
| `make flash-updater` | アップデータを書き込み |
| `make all` | flash + flash-updater |
| `make clean` | ビルド成果物を削除 |
| `make format` | clang-format-18 でソース整形 |
| `make test` | 全ユニットテスト実行 (55件) |
| `make test-<component>` | コンポーネント単位テスト (例: `make test-flash_fs`) |

## テスト

Catch2 v3 を使用。統合テストビルド（`test/CMakeLists.txt`）で Catch2 を一度だけコンパイルし、全テストで共有。

```bash
make test              # 全テスト
make test-osal         # osal だけ (22件)
make test-flash_fs     # flash_fs だけ (13件)
make test-shell        # shell だけ (9件)
make test-signal       # signal だけ (6件)
make test-flash_log    # flash_log だけ (4件)
make test-template     # template だけ (1件)
```

Python テスト（decode.py）:
```bash
cd components/log/tools && python3 -m pytest test_decode.py -v
```

## アーキテクチャ

```
apps/
├── main/           BLE スキャン → RSSI 収集 → Flash 記録 → OTA 転送
├── updater/        OTA ファームウェア書き込み
└── factory-test/   ハードウェア検査

components/         platform-independent ロジック + arch/ 層
├── ble/            BLE GAP (scan/advertise)
├── flash_fs/       NOR Flash ファイルシステム (stream + block)
├── shell/          UART シェル (help/ls/cat/erase)
├── log/            バイナリログ (flash_log) + テキストログ (log)
├── signal/         EMA フィルタ / RSSI 集約
├── osal/           RTOS 抽象化 (mutex/semaphore/task/timer)
├── ota/            OTA 受信・検証・書き込み
├── crash/          HardFault ハンドラ + crash info 永続化
├── sysconfig/      メモリマップ定数
├── drivers/        ハードウェアドライバ群
└── utkernel-cpp/   μT-Kernel C ラッパーヘッダ

kernel/             μT-Kernel 3 (setup.sh で取得)
linker/             リンカスクリプト
third_party/        nrfx, CMSIS
test/               統合テストビルド (Catch2)
```

## メモリレイアウト (Flash 512KB)

| 領域 | アドレス | サイズ |
|---|---|---|
| Application | `0x00000000` - `0x0006FFFF` | 448 KB |
| Flash FS (log) | `0x00070000` - `0x00073FFF` | 16 KB (4 pages) |
| Flash FS (config) | `0x00074000` - `0x00074FFF` | 4 KB (1 page) |
| Flash FS (ota_staging) | `0x00075000` - `0x00077FFF` | 12 KB (3 pages) |
| Updater | `0x00078000` - `0x0007DFFF` | 24 KB |
| Settings | `0x0007E000` - `0x0007EFFF` | 4 KB |
| MBR | `0x0007F000` - `0x0007FFFF` | 4 KB |

## Docker ビルド

```bash
docker build --build-arg ZIP_PASSWORD=<パスワード> -t ble-locator-fw .
```

## 命名規約

Rust-like naming convention を採用。詳細は `.github/copilot-instructions.md` を参照。

- 関数: `snake_case` (例: `flash_fs_init`, `ble_gap_discover`)
- 型/struct/enum: `PascalCase` (例: `FlashFsFileInfo`, `BLEGapEvent`)
- 定数/マクロ: `UPPER_SNAKE_CASE` (例: `PAGE_SIZE`, `LOG_E`)
