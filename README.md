# BLE屋内位置推定

Bluetooth Low Energy(BLE)の受信信号強度(RSSI)を収集し、
外部AIで機械学習ベースの位置推定を行う屋内測位アプリケーション。

## 3コンポーネント構成

| コンポーネント | 言語 | 役割 |
|---|---|---|
| **firmware/** | C++17 (μT-Kernel 3.0) | RTOSファームウェア: BLEスキャン・RSSI収集・位置追跡 |
| **host/** | Python | AIサーバ: RSSI→位置推定MLモデルの推論実行 |
| **smartphone/** | HTML/JS (Web Bluetooth) | スマホアプリ: BLE GATT経由で位置受信・フロアマップ表示 |

## アーキテクチャ

```
  [BLE Beacon A]  [BLE Beacon B]  [BLE Beacon C]  [BLE Beacon D]
       │               │               │               │
       └───── RSSI ─────┴───── RSSI ─────┴───── RSSI ────┘
                                │
       ┌──── マイコン (micro:bit v2.2 / μT-Kernel 3.0) ────┐
       │                                                     │
       │  [Observer]                  [Peripheral]           │
       │   BLEスキャン → RSSI収集      GATT Server           │
       │        ↓                        ↑                   │
       │   [μAI-Bridge]                  │                   │
       │        ↓                        │                   │
       │   位置推定結果 ────→ BLE通知 ───→│                   │
       │   モード管理 / エリア分析                            │
       └────────┼────────────────────────┼───────────────────┘
                │ TCP/UART               │ BLE GATT
                ▼                        ▼
       ┌────────────────┐      ┌──────────────────┐
       │ μAI-Host       │      │ Smartphone       │
       │ ble_locate     │      │ Web Bluetooth    │
       │ (Python ML)    │      │ Floor Map UI     │
       └────────────────┘      └──────────────────┘
```

## データフロー

1. **BLEスキャン**: マイコンが複数ビーコンのRSSIを周期的に収集（Observer役）
2. **AI推論**: μAI-Bridge経由でRSSI値をAIサーバに送信、座標(x,y)を推定
3. **BLE通知**: 推定位置をGATT Notificationでスマホに送信（Peripheral役）
4. **スマホ表示**: Web Bluetoothで受信し、フロアマップ上にリアルタイム表示

## BLE通信設計

マイコン（micro:bit nRF52833）は Observer と Peripheral の二重役を同時に実行:

| 役割 | 方向 | 用途 |
|---|---|---|
| **Observer** | ビーコン → マイコン | RSSI受信（パッシブスキャン） |
| **Peripheral (GATT Server)** | マイコン → スマホ | 位置通知・モード制御 |

### GATT サービス

| Characteristic | UUID | Properties | Format |
|---|---|---|---|
| Position | `...def1` | Notify | x(f32) y(f32) confidence(f32) = 12 bytes |
| Mode | `...def2` | Read/Write | mode(u8): 0=ACCURACY, 1=RESPONSIVE |

## 推定モード

| モード | サンプル数 | 間隔 | スムージング | 用途 |
|---|---|---|---|---|
| **ACCURACY** | 5回 | 2000ms | EMA α=0.3 | 正確性重視（見守り等） |
| **RESPONSIVE** | 1回 | 500ms | なし (α=1.0) | 応答性重視（リアルタイム追跡） |

信頼度のトレンドに基づいて自動切替も行う。

## ディレクトリ構成

```
ble-locator/
├── README.md
├── plan.md
│
├── firmware/                      # RTOSファームウェア（C++17）
│   ├── CMakeLists.txt
│   ├── main.cpp                   # メインループ（スキャン→推論→通知）
│   ├── app_config.h               # アプリ固定パラメータ
│   ├── ble_scanner.h/cpp          # BLEビーコンRSSIスキャン
│   ├── ble_service.h/cpp          # BLE GATTサービス（スマホ向け位置通知）
│   ├── location.h/cpp             # 位置管理・トラッキング・エリア分析
│   ├── estimation_mode.h/cpp      # 推定モード切替（正確性/応答性）
│   └── arch/                      # アーキテクチャ依存部
│       ├── arch.h                 # プラットフォーム抽象インターフェース
│       ├── sim/
│       │   └── arch_sim.cpp       # PCシミュレーション
│       └── microbit/
│           └── arch_microbit.cpp  # micro:bit v2.2実機
│
├── host/                          # AIサーバ（Python）
│   ├── uai_host.py                # μAI-Bridgeプロトコル推論サーバ
│   ├── model.py                   # 位置推定モデル（加重重心 / k-NNフィンガープリント）
│   └── requirements.txt
│
└── smartphone/                    # スマホアプリ（Web Bluetooth PWA）
    ├── app.py                     # Flask開発サーバ
    ├── requirements.txt
    └── templates/
        └── index.html             # BLE GATT Client + フロアマップUI
```

## ビルド・実行

```bash
# 1. AIサーバ起動
cd host && pip install -r requirements.txt && python uai_host.py --port 5000

# 2. ファームウェアビルド・実行 (SIM)
cd firmware/build && cmake .. && cmake --build . --config Debug
./Debug/ble_locator 127.0.0.1 5000

# 3. スマホアプリ起動 (開発用)
cd smartphone && pip install -r requirements.txt && python app.py
# → Android Chromeで http://<PC_IP>:8080 を開き、BLE接続
```
