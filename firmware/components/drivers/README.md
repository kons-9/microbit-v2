# Drivers コンポーネント

micro:bit v2.2 (nRF52833) の各種ハードウェアドライバと BLE GAP 層。

ハードウェアドライバは `namespace drivers` 内にクラスとして定義され、BLEのデータ型は `namespace ble`、操作APIは `drivers::Ble` として提供される。

---

## BLE GAP

nRF52833 の RADIO ペリフェラルを直接制御する BLE 層。SoftDevice は使用せず、スキャンと Advertising を提供する。

- 公開 API: `include/ble.h`
- 共通ロジック: `src/ble.cpp`
- micro:bit 実装: `src/arch/microbit/ble_microbit.cpp`

---

## Accelerometer (LSM303AGR 加速度計部)

- チップ: LSM303AGR (ST)
- インターフェース: I2C 内部バス (400kHz)
- SCL: P0.08, SDA: P0.16
- アドレス: 0x19
- 割り込み: COMBINED_SENSOR_INT P0.25
- レンジ: ±2g / ±4g / ±8g / ±16g
- 分解能: 12bit (high-resolution mode)
- WHO_AM_I: 0x33

---

## Magnetometer (LSM303AGR 磁気計部)

- チップ: LSM303AGR (ST) - 磁気計部
- インターフェース: I2C 内部バス (400kHz)
- SCL: P0.08, SDA: P0.16
- アドレス: 0x1E
- レンジ: ±50 gauss (固定)
- 分解能: 16bit (1.5 mgauss/LSB)
- WHO_AM_I: 0x40

### 備考
- I2Cバスは accelerometer と共有
- 統合時にはI2Cバスマネージャ経由でアクセスする設計を想定

---

## LED (5x5 マトリクス)

- 5行 x 5列 の LED マトリクス
- ROW pins (active high): P0.21, P0.22, P0.15, P0.24, P0.19
- COL pins (active low): P0.28, P0.11, P0.31, P1.05, P0.30
- ソフトウェア多重走査方式
- GPIO ドライブ強度: High Drive (H0H1) — 1行で5個同時駆動するため必須

### 設計メモ: なぜフレームバッファ + スキャンが必要か

マトリクス構造上、ROW/COL ピンが共有されているため物理的に1行しか同時点灯できない。
全行を表示するには時分割多重（スキャン）が不可避であり、`scan_tick()` を
定期タイマ (推奨 1ms) で継続的に呼び出す必要がある。スキャンを止めると表示が消える。

フレームバッファはスキャンが毎 tick 参照する「表示したい状態」のキャッシュ。

---

## Speaker

- チップ: JIANGSU HUANENG MLT-8530
- 接続: P0.00 (PWM出力)
- 自己共振周波数: 2700Hz
- SPL: 80dB @ 5V, 10cm

---

## Microphone (SPU0410LR5H)

- チップ: Knowles SPU0410LR5H-QB-7 MEMS
- MIC_IN: P0.05 (ADC AIN3)
- RUN_MIC: P0.20 (電源制御, HIGH=有効)
- 感度: -38dB ±3dB @ 94dB SPL
- SNR: 63dB
- 周波数範囲: 100Hz ~ 80kHz

### 既知の障害: SAADC と GPIO の干渉 (P0.28 / LED COL1)

#### 現象
`nrfx_saadc_simple_mode_set()` を呼ぶと、LED マトリクスの COL1 (P0.28 = AIN4) が
GPIO 出力として機能しなくなり、左端1列が消灯する。
コメントアウトすると正常に動作する。

#### 確認済みの事実
- SAADC チャネル設定は AIN3 (P0.05) のみ。PSELP に AIN4 は設定していない
- P0.28 = AIN4 (nRF52833 ピンマップ)
- `nrfx_saadc_init()` + `nrfx_saadc_channel_config()` だけでは発生しない
- `nrfx_saadc_simple_mode_set()` を追加すると発生する
- `nrfx_saadc_uninit()` 後は P0.28 の GPIO 出力が復帰する

#### 仕様上の記述
nRF52833 Product Specification — SAADC ENABLE レジスタ:

> "When enabled, the SAADC will acquire access to analog input pins specified
> in registers CH[n].PSELP and CH[n].PSELN"

仕様通りなら AIN3 のみが影響を受けるはずだが、実際には AIN4 (P0.28) も影響される。
**仕様では説明がつかない現象であり、根本原因は未特定。**

#### 参考情報
- nRF52833 Product Specification — SAADC
  https://docs.nordicsemi.com/bundle/ps_nrf52833/page/saadc.html
- nrfx ドライバ内部で `saadc_generic_mode_set()` が全8チャネルの PSELP レジスタに
  書き込みを行う（未使用チャネルには NC=0 を書く）
- anomaly 212 workaround が SAADC を power cycle する処理あり

#### 対策
SAADC を常駐させず、`read()` のたびに init → sample → uninit する。
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

---

## Button

- Button A: P0.14 (active low, 外部 4.7K プルアップ)
- Button B: P0.23 (active low, 外部 4.7K プルアップ)
- デバウンス: ソフトウェア 50ms
- ポーリング方式 (μT-Kernel `tk_dly_tsk` 使用)
- 外部プルアップ付きのため `NRF_GPIO_PIN_NOPULL` で設定

---

## Touch (タッチロゴ)

- FACE_TOUCH: P1.04
- 方式: 静電容量式 (10Mohm プルアップによるRC時定数検出)
- タッチ時: LOW / 非タッチ時: HIGH
- ポーリング方式 (μT-Kernel `tk_dly_tsk` 使用)

---

## Temperature (nRF52833 内蔵)

- センサ: nRF52833 内蔵 TEMP peripheral
- レンジ: -40°C ~ 105°C
- 分解能: 0.25°C
- 精度: ±5°C (未キャリブレーション)
- 測定時間: ~36μs
- 測定はオンデマンド (TASKS_START/EVENTS_DATARDY)
- チップ内部温度のため、環境温度より数度高くなる傾向あり

---

## UART

- nRF52833 UARTE0 ペリフェラル
- TX: P0.06 → USB Interface MCU → USB CDC
- RX: P1.08 ← USB Interface MCU ← USB CDC
- フロー制御なし
- デフォルト: 115200bps
