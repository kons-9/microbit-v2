# BLE Locator — 開発計画

## ゴール

BLE(Bluetooth Low Energy)のRSSIを複数ノードから収集し、
外部AIで機械学習ベースの位置推定を行う屋内測位アプリケーション。

## 現状

| コンポーネント | 状態 |
|---|---|
| **firmware (C++)** | |
| 　main.cpp (メインループ) | ✅ 実装済み |
| 　ble_scanner (BLEスキャン) | ✅ 実装済み |
| 　ble_service (GATT配信) | ✅ 実装済み |
| 　location (位置追跡) | ✅ 実装済み |
| 　estimation_mode (モード切替) | ✅ 実装済み |
| 　arch/sim (PCシミュレーション) | ✅ 実装済み |
| 　arch/microbit (micro:bit) | 🔲 スケルトンのみ |
| **host (Python)** | |
| 　uai_host.py (AIサーバ) | ✅ 実装済み |
| 　model.py (ML推定モデル) | ✅ 実装済み |
| **smartphone (Web)** | |
| 　index.html (フロアマップUI) | ✅ 実装済み |
| **テスト** | ❌ なし |

## TODO

### すぐやる

- [ ] テスト追加（firmware シミュレーション、host ユニットテスト）
- [ ] host にモデル学習用サンプルデータを追加（training/）
- [ ] .gitignore 拡充

### ハードウェア到着後

- [ ] micro:bit v2.2 でのBLEスキャン動作確認
- [ ] arch/microbit の実装（BLE 5.1 API統合）
- [ ] 複数micro:bit でのRSSI収集・位置推定の端到端テスト

### 将来

- [ ] BLE AoA (Angle of Arrival) 対応
- [ ] フロアマップのリアルタイム更新（WebSocket）
- [ ] 学習済みモデルの自動チューニング
