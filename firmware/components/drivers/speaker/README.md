# Speaker コンポーネント

micro:bit v2.2 のスピーカードライバ。

## ハードウェア
- チップ: JIANGSU HUANENG MLT-8530
- 接続: P0.00 (PWM出力)
- 自己共振周波数: 2700Hz
- SPL: 80dB @ 5V, 10cm

## API
- `speaker_init()` - 初期化（PWM設定）
- `speaker_tone(freq_hz)` - 指定周波数で再生
- `speaker_stop()` - 停止
- `speaker_is_playing()` - 再生状態確認


