# components

micro:bit v2.2 向けコンポーネント群。各コンポーネントは arch/ 層で実機/Linuxを切り替える。

## コンポーネント一覧

| コンポーネント | 説明 | テスト |
|---|---|---|
| `drivers` | BLE GAP、UART、各種ハードウェアドライバ | - |
| `fs` | NOR Flash ファイルシステム (ring buffer + fixed file) | 16件 |
| `shell` | UART シェル (help/ls/cat/erase) | 10件 |
| `log` | 複数の `io::Stream` へのテキストログ出力 | 17件 |
| `sysconfig` | メモリマップ・ブート切り替え | - |
| `utkernel-cpp` | RAII対応のμT-Kernel C++抽象化 | 22件 |
| `template` | 新規コンポーネントのテンプレート | 1件 |

### ドライバ (`drivers/`)

BLE GAP (scan/advertise) も、RADIOのアーキテクチャ実装を含めてこのコンポーネントに統合しています。

| ドライバ | チップ/ペリフェラル | インターフェース |
|---|---|---|
| `accelerometer` | LSM303AGR | I2C 0x19 |
| `magnetometer` | LSM303AGR | I2C 0x1E |
| `led` | 5x5 LED マトリクス | GPIO ROW/COL 走査 |
| `speaker` | MLT-8530 | PWM P0.00 |
| `mic` | SPU0410LR5H | ADC P0.05 + GPIO P0.20 |
| `button` | タクトスイッチ A/B | GPIO P0.14/P0.23 |
| `touch` | 静電容量パッド | GPIO P1.04 |
| `temperature` | nRF52833 内蔵 | TEMP peripheral |
| `uart` | nRF52833 UARTE0 | USB CDC 経由 |

## テスト実行

```bash
cd firmware
make test              # 全 55 件
make test-fs     # コンポーネント単位
make test-utkernel
make test-shell
```

## アーキテクチャパターン

```
component_name/
├── CMakeLists.txt
├── include/
│   └── component_name.h    # 公開 API
├── src/
│   ├── component_name.cpp  # プラットフォーム非依存ロジック
│   └── arch/
│       ├── microbit/       # nRF52833 実機実装
│       └── linux/          # ホストテスト用スタブ
└── test/
    └── linux/              # Catch2 ユニットテスト
```

## ピンマップ (micro:bit v2.2)

```
P0.00  SPEAKER (PWM)          P0.21  ROW1
P0.05  MIC_IN (ADC AIN3)      P0.22  ROW2
P0.06  UART_TX                 P0.15  ROW3
P0.08  I2C_SCL                 P0.24  ROW4
P0.11  COL2                    P0.19  ROW5
P0.14  BUTTON_A               P0.28  COL1
P0.16  I2C_SDA                P0.31  COL3
P0.20  RUN_MIC                P1.05  COL4
P0.23  BUTTON_B               P0.30  COL5
P0.25  SENSOR_INT             P1.04  FACE_TOUCH
P1.08  UART_RX
```
