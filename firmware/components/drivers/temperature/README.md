# Temperature コンポーネント

micro:bit v2.2 の温度センサドライバ (nRF52833 内蔵)。

## ハードウェア
- センサ: nRF52833 内蔵 TEMP peripheral
- レンジ: -40°C ~ 105°C
- 分解能: 0.25°C
- 精度: ±5°C (未キャリブレーション)
- 測定時間: ~36μs

## API
- `temperature_init()` - 初期化
- `temperature_read()` - 温度取得 (°C, 整数)
- `temperature_read_raw()` - 生値取得 (0.25°C単位, 例: 100 = 25.0°C)

## 備考
- 測定はオンデマンド (TASKS_START/EVENTS_DATARDY)
- チップ内部温度のため、環境温度より数度高くなる傾向あり

## ビルド
```
cmake -B build/test -DTARGET_ARCH=linux
cmake --build build/test
ctest --test-dir build/test
```
