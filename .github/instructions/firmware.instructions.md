---
applyTo: "firmware/**"
---

# BLE Locator

## プロジェクト概要

BLE(Bluetooth Low Energy)のRSSIを複数ノードから収集し、
外部AIサーバで機械学習ベースの位置推定を行う屋内測位アプリケーション。

## 設計原則
- Observer + Peripheral二重役（スキャン＋結果配信を同時に行う）
- 推定モード切替: ACCURACY（正確性重視）/ RESPONSIVE（応答性重視）

## 既知の注意事項
- TDDで開発すること。テストコードも提出すること。
- testはCatch2またはGoogleTestを使用すること
- マイコン側はC++20
- ISR（割り込みハンドラ）コンテキストから `LOG_*` マクロ（`LogOutput`）を呼んではならない。UART 送信がブロッキング（busy-wait）であり、ロックの取得もできないため。
- TIMER1 は BLE アドバタイザ（`ble_microbit.cpp` が直接レジスタ操作）で使用済み。LED スキャンなど他用途には TIMER2 以降を使うこと。

（ここに開発中に見つけた注意事項を追記する）
