# LED コンポーネント

micro:bit v2.2 の 5x5 LED マトリクスドライバ。

## ハードウェア
- 5行 x 5列 の LED マトリクス
- ROW pins (active high): P0.21, P0.22, P0.15, P0.24, P0.19
- COL pins (active low): P0.28, P0.11, P0.31, P1.05, P0.30
- ソフトウェア多重走査方式

## API
- `led_init()` - 初期化
- `led_set(row, col, on)` - 個別LED制御
- `led_clear()` - 全消灯
- `led_set_frame(bitmap)` - フレームバッファ一括設定
- `led_scan_tick()` - 走査（定期タイマから呼び出し）


