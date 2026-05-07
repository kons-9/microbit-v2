# components

micro:bit v2.2 ハードウェアドライバ群。

各コンポーネントのテンプレートは ./template を参照。

## コンポーネント一覧

| コンポーネント | 説明 | インターフェース | チップ/ペリフェラル |
|---|---|---|---|
| `led` | 5x5 LEDマトリクス | GPIO (ROW/COL走査) | 表面実装LED |
| `speaker` | スピーカー | PWM (P0.00) | MLT-8530 |
| `mic` | マイク | ADC (P0.05) + GPIO (P0.20) | SPU0410LR5H-QB-7 |
| `accelerometer` | 加速度センサ | I2C 0x19 | LSM303AGR |
| `magnetometer` | 地磁気センサ | I2C 0x1E | LSM303AGR |
| `button` | ボタンA/B | GPIO (P0.14/P0.23) | タクトスイッチ |
| `touch` | タッチロゴ | GPIO (P1.04) | 静電容量式パッド |
| `temperature` | 温度センサ | TEMP peripheral | nRF52833内蔵 |
| `ble` | BLE通信 | SoftDevice | nRF52833 Radio |
| `osal` | OS抽象化層 | - | μT-Kernel |
| `utkernel-cpp` | μT-Kernel C++ラッパー | - | - |
| `template` | コンポーネントテンプレート | - | - |

## ディレクトリ構成 (各コンポーネント共通)

```
component_name/
├── CMakeLists.txt          # ビルド定義
├── README.md               # ドキュメント
├── include/
│   └── component_name.h    # 公開API (C/C++共用)
├── src/
│   ├── component_name.cpp  # プラットフォーム非依存ロジック
│   └── arch/
│       ├── microbit/       # nRF52833 実機実装
│       └── linux/          # ホストテスト用スタブ
└── test/
    └── linux/              # Linux上ユニットテスト
```

## ピンマップ (micro:bit v2.2 / nRF52833)

```
P0.00  SPEAKER (PWM)
P0.05  MIC_IN (ADC AIN3)
P0.08  I2C_INT_SCL (加速度/地磁気)
P0.14  BUTTON_A (active low)
P0.15  ROW3
P0.16  I2C_INT_SDA (加速度/地磁気)
P0.19  ROW5
P0.20  RUN_MIC (マイク電源)
P0.21  ROW1
P0.22  ROW2
P0.23  BUTTON_B (active low)
P0.24  ROW4
P0.25  COMBINED_SENSOR_INT
P0.28  COL1
P0.30  COL5
P0.31  COL3
P1.04  FACE_TOUCH
P1.05  COL4
P0.11  COL2
```

## ビルド方法

### Linux テスト
```bash
cmake -B build/test -DTARGET_ARCH=linux
cmake --build build/test
ctest --test-dir build/test
```

### ファームウェア (microbit)
```bash
cmake -B build/fw -DTARGET_ARCH=microbit
cmake --build build/fw
```
