---
applyTo: "**"
---

# BLE Locator

## プロジェクト概要

BLE(Bluetooth Low Energy)のRSSIを複数ノードから収集し、
外部AIサーバで機械学習ベースの位置推定を行う屋内測位アプリケーション。

## アーキテクチャ

3コンポーネント構成:
- **firmware/** — μT-Kernel 3.0上のRTOSファームウェア（BLEスキャン・RSSI収集・位置追跡）
- **host/** — AIサーバ（RSSI→位置推定MLモデルの推論実行）
- **smartphone/** — スマホWebアプリ（BLE GATT経由で位置受信・フロアマップ表示）

## 設計原則

- micro:bit v2.2のBLE 5.1を使用
- Observer + Peripheral二重役（スキャン＋結果配信を同時に行う）
- 推定モード切替: ACCURACY（正確性重視）/ RESPONSIVE（応答性重視）
- μAI-Bridgeの通信抽象化により、AI推論先を透過的に切替可能

## 実装方針

- **firmware**: C++17。μAI-Bridge依存
- **host**: Python + scikit-learn
- **smartphone**: HTML/JS (Web Bluetooth API)

## ビルド

```bash
cd firmware
cmake -B build -DUAI_PLATFORM=SIM
cmake --build build
```

## 依存関係

- [μAI-Bridge](../uai-bridge/) — 通信ミドルウェア

## 既知の注意事項

（ここに開発中に見つけた注意事項を追記する）
