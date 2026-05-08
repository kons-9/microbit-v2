# BLE屋内位置推定

Bluetooth Low Energy (BLE) の受信信号強度 (RSSI) を収集し、機械学習ベースの位置推定を行う屋内測位システム。

## システム概要

micro:bit v2.2 (nRF52833) をスキャナ端末として使用し、周囲の BLE ビーコンの RSSI を収集・記録する。
記録データは OTA またはシェル経由で取得し、外部 AI で位置推定モデルを学習・推論する。

## 推定モード

| モード | サンプル数 | 間隔 | スムージング | 用途 |
|---|---|---|---|---|
| **ACCURACY** | 5回 | 2000ms | EMA α=0.3 | 正確性重視（見守り等） |
| **RESPONSIVE** | 1回 | 500ms | なし (α=1.0) | 応答性重視（リアルタイム追跡） |

## クイックスタート

```bash
cd firmware
make build      # ファームウェアビルド
make flash      # micro:bit に書き込み
make test       # ユニットテスト実行 (55件, Catch2)
```

詳細は [firmware/README.md](firmware/README.md) を参照。

## 開発環境

- WSL2 (Ubuntu)
- gcc-arm-none-eabi / CMake 3.16+
- Python 3 + pyocd（書き込み）
- Catch2 v3（テスト）

## ディレクトリ構成

```
ble-locator/
├── firmware/       ファームウェア本体 (C/C++20, μT-Kernel 3)
├── hooks/          Git hooks (pre-commit, pre-push)
└── wsl.md          WSL セットアップ手順
```