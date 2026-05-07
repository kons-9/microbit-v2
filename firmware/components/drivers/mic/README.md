# Mic コンポーネント

micro:bit v2.2 のマイクドライバ。

## ハードウェア
- チップ: Knowles SPU0410LR5H-QB-7 MEMS
- MIC_IN: P0.05 (ADC AIN3)
- RUN_MIC: P0.20 (電源制御, HIGH=有効)
- 感度: -38dB ±3dB @ 94dB SPL
- SNR: 63dB
- 周波数範囲: 100Hz ~ 80kHz

## API
- `mic_init()` - 初期化（ADC/GPIO設定）
- `mic_enable()` / `mic_disable()` - 電源制御
- `mic_is_enabled()` - 状態確認
- `mic_read()` - ADC生値取得 (0-1023)
- `mic_get_level()` - 音量レベル取得 (0-255)

## ビルド
```
cmake -B build/test -DTARGET_ARCH=linux
cmake --build build/test
ctest --test-dir build/test
```
