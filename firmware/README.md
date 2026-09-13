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
| `make build` | SoftDeviceなしのメインアプリをビルド |
| `make build-recovery` | リカバリー用ファームウェアをビルド |
| `make build-integration-test` | 実機向け統合テストをビルド |
| `make flash` | メインアプリを書き込み |
| `make flash-recovery` | リカバリー用ファームウェアを書き込み |
| `make flash-integration-test` | 実機向け統合テストを書き込み |
| `make all` | flash + flash-recovery |
| `make clean` | ビルド成果物を削除 |
| `make format` | clang-format-18 でソース整形 |
| `make test` | 全ユニットテスト実行 (74件) |
| `make test-<component>` | コンポーネント単位テスト (例: `make test-fs`) |

## テスト

Catch2 v3 を使用。統合テストビルド（`test/CMakeLists.txt`）で Catch2 を一度だけコンパイルし、全テストで共有。

```bash
make test              # 全テスト (74件)
make test-utkernel      # utkernel-cpp だけ (22件)
make test-fs            # fs だけ (16件)
make test-shell        # shell だけ (10件)
make test-template     # template だけ (1件)
```

## アーキテクチャ

```
apps/
└── microbit/
    ├── main/           BLE Advertisingメインアプリ
    │   ├── crash/      HardFaultハンドラ + crash info永続化
    │   └── task/       EntryTask、BLE Advertising task
    ├── recovery/       緊急時の不揮発ログ読み出し用 shell
    ├── integration-test/ 統合テスト
    └── sample/         サンプルアプリ

components/         platform-independent ロジック + arch/ 層
├── fs/       NOR Flash ファイルシステム (ring buffer + fixed file)
├── shell/          UART シェル (help/ls/cat/erase)
├── log/            テキストログとログ出力インターフェース
├── sysconfig/      メモリマップ定数
├── drivers/        BLE GAP、UART、ハードウェアドライバ群
└── utkernel-cpp/   RAII対応のμT-Kernel C++抽象化

kernel/             μT-Kernel 3 (setup.sh で取得)
linker/             リンカスクリプト
third_party/        nrfx, CMSIS
test/               統合テストビルド (Catch2)
```

## メモリレイアウト (Flash 512KB)

| 領域 | アドレス | サイズ |
|---|---|---|
| Application | `0x00000000` - `0x00067FFF` | 416 KB |
| Recovery | `0x00068000` - `0x00077FFF` | 64 KB |
| Flash FS (log) | `0x00078000` - `0x0007BFFF` | 16 KB (4 pages) |
| Flash FS (settings) | `0x0007C000` - `0x0007CFFF` | 4 KB (1 page) |
| Flash FS (calib) | `0x0007D000` - `0x0007DFFF` | 4 KB (1 page) |
| Reserved | `0x0007E000` - `0x0007EFFF` | 4 KB |
| Settings | `0x0007F000` - `0x0007FFFF` | 4 KB |

## Docker ビルド

```bash
docker build --build-arg ZIP_PASSWORD=<パスワード> -t ble-locator-fw .
```

## 命名規約

Rust-like naming convention を採用。詳細は `.github/copilot-instructions.md` を参照。

- 関数: `snake_case` (例: `fs_init`, `ble_gap_discover`)
- 型/struct/enum: `PascalCase` (例: `FlashFsFileInfo`, `BLEGapEvent`)
- 定数/マクロ: `UPPER_SNAKE_CASE` (例: `PAGE_SIZE`, `LOG_E`)
