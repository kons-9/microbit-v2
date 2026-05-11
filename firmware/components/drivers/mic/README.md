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
- `microphone_init()` - 初期化（GPIO のみ、SAADC は触らない）
- `microphone_enable()` / `microphone_disable()` - 電源制御
- `microphone_is_enabled()` - 状態確認
- `microphone_read()` - ADC生値取得 (0-1023)
- `microphone_get_level()` - 音量レベル取得 (0-255)

## 既知の障害: SAADC と GPIO の干渉 (P0.28 / LED COL1)

### 現象
`nrfx_saadc_simple_mode_set()` を呼ぶと、LED マトリクスの COL1 (P0.28 = AIN4) が
GPIO 出力として機能しなくなり、左端1列が消灯する。
コメントアウトすると正常に動作する。

### 確認済みの事実
- SAADC チャネル設定は AIN3 (P0.05) のみ。PSELP に AIN4 は設定していない
- P0.28 = AIN4 (nRF52833 ピンマップ)
- `nrfx_saadc_init()` + `nrfx_saadc_channel_config()` だけでは発生しない
- `nrfx_saadc_simple_mode_set()` を追加すると発生する
- `nrfx_saadc_uninit()` 後は P0.28 の GPIO 出力が復帰する

### 仕様上の記述
nRF52833 Product Specification — SAADC ENABLE レジスタ:

> "When enabled, the SAADC will acquire access to analog input pins specified
> in registers CH[n].PSELP and CH[n].PSELN"

仕様通りなら AIN3 のみが影響を受けるはずだが、実際には AIN4 (P0.28) も影響される。
**仕様では説明がつかない現象であり、根本原因は未特定。**

### 参考情報
- nRF52833 Product Specification — SAADC
  https://docs.nordicsemi.com/bundle/ps_nrf52833/page/saadc.html
- nrfx ドライバ内部で `saadc_generic_mode_set()` が全8チャネルの PSELP レジスタに
  書き込みを行う（未使用チャネルには NC=0 を書く）
- anomaly 212 workaround が SAADC を power cycle する処理あり

### 対策
SAADC を常駐させず、`microphone_read()` のたびに init → sample → uninit する。
SAADC が disabled の間は P0.28 の GPIO 出力に干渉しない（実験で確認済み）。

```
saadc_sample_once():
  nrfx_saadc_init()
  nrfx_saadc_channel_config()
  nrfx_saadc_simple_mode_set()
  nrfx_saadc_buffer_set()
  nrfx_saadc_mode_trigger()
  → 結果取得
  nrfx_saadc_uninit()        ← これで SAADC disabled に戻る
```

オーバーヘッドは数十 µs/回だが、マイク読み取りは低頻度のため許容範囲。


