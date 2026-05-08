# Magnetometer コンポーネント

micro:bit v2.2 の地磁気センサドライバ (LSM303AGR 磁気計部)。

## ハードウェア
- チップ: LSM303AGR (ST) - 磁気計部
- インターフェース: I2C 内部バス (400kHz)
- SCL: P0.08, SDA: P0.16
- アドレス: 0x1E
- レンジ: ±50 gauss (固定)
- 分解能: 16bit (1.5 mgauss/LSB)
- WHO_AM_I: 0x40

## API
- `magnetometer_init()` - 初期化（I2C設定、WHO_AM_I確認）
- `magnetometer_read()` - 3軸磁場データ取得 (mGauss単位)
- `magnetometer_who_am_i()` - デバイスID確認 (期待値: 0x40)
- `magnetometer_heading()` - 方位角取得 (0-359度, 北=0)

## 備考
- I2Cバスは `accelerometer` コンポーネントと共有
- 統合時にはI2Cバスマネージャ経由でアクセスする設計を想定


