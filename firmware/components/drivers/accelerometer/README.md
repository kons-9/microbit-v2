# Accelerometer コンポーネント

micro:bit v2.2 の加速度センサドライバ (LSM303AGR)。

## ハードウェア
- チップ: LSM303AGR (ST)
- インターフェース: I2C 内部バス (400kHz)
- SCL: P0.08, SDA: P0.16
- 加速度計アドレス: 0x19
- 地磁気計アドレス: 0x1E (本ドライバでは加速度のみ)
- 割り込み: COMBINED_SENSOR_INT P0.25
- レンジ: ±2g / ±4g / ±8g / ±16g
- 分解能: 12bit (high-resolution mode)

## API
- `accel_init()` - 初期化（I2C設定、WHO_AM_I確認）
- `accel_set_range(range)` - 測定レンジ設定
- `accel_read()` - 3軸加速度データ取得 (mg単位)
- `accel_who_am_i()` - デバイスID確認 (期待値: 0x33)

## ビルド
```
cmake -B build/test -DTARGET_ARCH=linux
cmake --build build/test
ctest --test-dir build/test
```
