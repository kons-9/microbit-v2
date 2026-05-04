---
applyTo: "**"
---

# BLE Locator

## プロジェクト概要

BLE(Bluetooth Low Energy)のRSSIを複数ノードから収集し、
外部AIサーバで機械学習ベースの位置推定を行う屋内測位アプリケーション。

## 設計原則

- micro:bit v2.2のBLE 5.1を使用
- Observer + Peripheral二重役（スキャン＋結果配信を同時に行う）
- 推定モード切替: ACCURACY（正確性重視）/ RESPONSIVE（応答性重視）
- μAI-Bridgeの通信抽象化により、AI推論先を透過的に切替可能

## 既知の注意事項

（ここに開発中に見つけた注意事項を追記する）
