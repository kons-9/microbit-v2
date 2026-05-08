# Signal コンポーネント

BLE RSSI 信号処理ユーティリティ。プラットフォーム非依存。

## 機能

### EMA (Exponential Moving Average) フィルタ
- RSSI のスムージングに使用
- ACCURACY モード: α=0.3（ノイズ除去重視）
- RESPONSIVE モード: α=1.0（スムージングなし＝即応）

### RSSI 集約器
- 複数サンプルの平均を計算
- ACCURACY モード: 5サンプル集約
- RESPONSIVE モード: 1サンプル（集約なし）

## API

- `signal_ema_init(ema, alpha)` - EMA フィルタ初期化
- `signal_ema_update(ema, sample)` - サンプル入力・フィルタ出力取得
- `signal_ema_reset(ema)` - フィルタリセット
- `signal_rssi_accum_init(accum, capacity)` - RSSI 集約器初期化
- `signal_rssi_accum_add(accum, rssi)` - サンプル追加
- `signal_rssi_accum_average(accum)` - 平均 RSSI 取得
- `signal_rssi_accum_reset(accum)` - リセット

## テスト

```bash
cd firmware && make test-signal   # 6 件
```
