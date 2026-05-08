# firmware

micro:bit v2.2 (nRF52833) 向けファームウェア。μT-Kernel 3 RTOS 上で動作する BLE ロケータ。

## 前提条件

- `gcc-arm-none-eabi` (GNU Arm Embedded Toolchain)
- CMake 3.16+
- Python 3 + venv（書き込みに pyocd を使用）
- `wget`, `unzip`

## セットアップ

```bash
# サブモジュール取得
git submodule update --init --recursive

# μT-Kernel 3 のダウンロード・展開（パスワードは下記URLを参照）
# https://www.t-engine4u.com/info/mbit/2.html
cd kernel
./setup.sh
cd ..
```

## ビルド

```bash
# メインアプリ
make build

# アップデータ
make build-updater

# ビルド成果物を削除
make clean
```

## 書き込み

```bash
# メインアプリ + アップデータを書き込み（初回は自動でフラッシュ消去）
make all

# 個別に書き込み
make flash
make flash-updater
```

## フォーマット

```bash
make format   # clang-format-18 でソースを整形
```

## Docker ビルド

```bash
docker build --build-arg ZIP_PASSWORD=<パスワード> -t ble-locator-fw .
```

## ディレクトリ構成

```
firmware/
├── Makefile                 # ビルド・書き込みの統合ターゲット
├── CMakeLists.txt           # トップレベル CMake（APP_TARGET で切替）
├── Dockerfile               # Docker ビルド環境
├── requirements.txt         # Python依存 (pyocd等)
├── cmake/
│   └── toolchain.cmake      # arm-none-eabi クロスコンパイル設定
├── apps/
│   ├── main/                # メインアプリケーション
│   ├── updater/             # OTA アップデータ
│   └── factory-test/        # 工場テスト
├── components/              # ハードウェアドライバ・ミドルウェア
├── kernel/                  # μT-Kernel 3 (setup.sh で取得)
├── linker/                  # リンカスクリプト
│   ├── memory_map.ld        # メモリレイアウト定義
│   ├── app.ld               # メインアプリ用
│   └── updater.ld           # アップデータ用
└── third_party/             # 外部ライブラリ (nrfx, CMSIS)
```

## メモリレイアウト (Flash 512KB)

| 領域 | アドレス | サイズ |
|---|---|---|
| SoftDevice S140 | `0x00000000` - `0x00025FFF` | 152 KB |
| Application | `0x00026000` - `0x0006DFFF` | 288 KB |
| Updater | `0x0006E000` - `0x00077FFF` | 40 KB |
| Bootloader | `0x00078000` - `0x0007EFFF` | 28 KB |
| Settings | `0x0007F000` - `0x0007FFFF` | 4 KB |

## アプリケーション切替

`CMakeLists.txt` の `APP_TARGET` で対象アプリを選択:

```bash
cmake -B build -DAPP_TARGET=main          # メインアプリ (デフォルト)
cmake -B build -DAPP_TARGET=updater       # アップデータ
cmake -B build -DAPP_TARGET=factory-test  # 工場テスト
```
