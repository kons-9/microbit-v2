# BLE コンポーネント

nRF52833 の RADIO ペリフェラルを直接操作する BLE GAP 層。

## 機能

- **スキャン**: 周囲の BLE Advertisement をパッシブ受信し、アドレス・RSSI をコールバック通知
- **アドバタイズ**: カスタム PDU を 3ch (37/38/39) で送信

## API

- `ble_init()` - RADIO 初期化
- `ble_gap_discover(params, callback)` - スキャン開始
- `ble_gap_discover_cancel()` - スキャン停止
- `ble_gap_discovery_active()` - スキャン中か確認
- `ble_gap_advertise_set_data(data, len)` - アドバタイズデータ設定
- `ble_gap_advertise_start(params)` - アドバタイズ開始
- `ble_gap_advertise_stop()` - アドバタイズ停止

## 設計

SoftDevice を使用せず、RADIO ペリフェラルをベアメタルで制御。
割り込みハンドラ (`RADIO_IRQHandler`) でパケット受信を処理し、ユーザコールバックへ通知する。