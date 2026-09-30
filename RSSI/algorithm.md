# RSSI ベース自己位置推定：基礎から応用まで

本文書は survey.md に収集した論文群に基づき、RSSI（Received Signal Strength Indicator）を用いた屋内自己位置推定の全体像を、基礎原理からアルゴリズム、実装上の課題、応用までを体系的にまとめたものである。

### 用語と問題設定

本文書および survey.md における分類は **受信機（推定対象）の数** に基づく：

| 用語 | 意味 |
|------|------|
| **1 ビーコン** | 複数の固定送信機（BLE ビーコン等）が存在する環境で、**推定対象の受信機が 1 台**。標準的な自己位置推定問題。 |
| **複数ビーコン** | 同一環境で **推定対象の受信機が複数台** 同時に存在する。相互干渉・協調推定・スケーラビリティが追加課題となる。 |

いずれの場合も、位置既知の **送信機（アンカー）は複数** 設置されている前提である。受信機は各送信機からの RSSI を観測し、それを手がかりに自身の位置を推定する。

---

## 1. RSSI 測位の基本原理

### 1.1 電波伝搬モデル（Log-Distance Path-Loss Model）

RSSI から距離を推定する根幹は、Rappaport (1996/2002) が体系化した **対数距離経路損失モデル** である。

$$
RSSI(d) = RSSI(d_0) - 10 \, n \, \log_{10}\!\left(\frac{d}{d_0}\right) + X_\sigma
$$

| 記号 | 意味 |
|------|------|
| $d$ | 送信機からの距離 |
| $d_0$ | 基準距離（通常 1 m） |
| $RSSI(d_0)$ | 基準距離での受信電力（実測 or データシートから取得） |
| $n$ | 経路損失指数（自由空間: 2、屋内: 2〜4） |
| $X_\sigma$ | 対数正規シャドウイング（平均 0、標準偏差 $\sigma$ dB のガウス雑音） |

このモデルを逆算すると距離推定式が得られる：

$$
\hat{d} = d_0 \cdot 10^{\frac{RSSI(d_0) - RSSI_{\text{measured}}}{10n}}
$$

**課題**: $X_\sigma$ の存在により、同一距離でも RSSI は数 dB〜10 dB 以上揺らぐ。これが RSSI 測位の本質的な精度限界であり、以降のすべてのアルゴリズムはこのノイズへの対処が主題となる。

> 参考: Cinefra (2012) が BLE における校正手順と誤差要因を詳述。

---

## 2. 測位手法の大分類

Leitch et al. (2023) のサーベイおよび RADAR (2000) に基づき、RSSI 測位は以下の 2 系統に大別される。

| 分類 | 原理 | 長所 | 短所 |
|------|------|------|------|
| **幾何学的手法**（測距＋三辺測量） | RSSI → 距離 → 位置を計算 | オンライン、事前データ不要 | path-loss モデルの精度に依存 |
| **フィンガープリント法**（シーン解析） | 事前に各地点の RSSI マップを作成し照合 | 環境固有の伝搬特性を吸収 | オフライン収集が大コスト、環境変化に弱い |

### 2.1 幾何学的手法（Trilateration / Multilateration）

受信機が複数の送信機から RSSI を取得し、各送信機への距離を推定。送信機を中心とする円（2D）や球（3D）の交点として受信機位置を求める。

- **三辺測量（Trilateration）**: 最低 3 基の送信機からの距離が必要（2D の場合）。
- **重み付き三辺測量**: 各送信機からの RSSI の信頼度（分散の逆数等）で重み付け → Cantón Paterna et al. (2017) が BLE で実装。
- **最小二乗法**: 送信機が 3 基以上ある場合、過決定連立方程式を最小二乗で解くことで精度向上。

### 2.2 フィンガープリント法

1. **オフラインフェーズ**: 環境内の各参照点で、全送信機からの RSSI ベクトルを収集しデータベース化。
2. **オンラインフェーズ**: 受信機が測定した現在の RSSI ベクトルとデータベースを照合し、最も類似する参照点を推定位置とする。

照合手法:
- **決定論的**: k-NN（RADAR, 2000）
- **確率的**: 各参照点の RSSI 分布をモデル化し最尤推定（Horus, 2005）

---

## 3. 単一受信機の自己位置推定（1 ビーコン問題）

受信機 1 台が、環境に設置された複数の送信機からの RSSI を用いて自身の位置を推定する。最も基本的な構成であり、survey.md の「1 ビーコン」セクションに該当する。

### 3.1 観測情報による分類

受信機が得られる情報の種類によって、適用可能な手法が異なる。

| 利用可能な観測 | 幾何学的意味 | 最低限必要な送信機数 (2D) |
|------|------|------|
| RSSI のみ（距離のみ） | 各送信機を中心とする円 | 3 基（三辺測量） |
| RSSI + AoA（距離＋角度） | 各送信機から方位付き距離 | 1 基で一意に決定可能 |

### 3.2 RSSI のみの場合（標準構成）

複数送信機からの RSSI を受信し、三辺測量・最小二乗法・フィンガープリント法で位置を求める。これが最も一般的な構成。

**精度向上のためのアプローチ**:
- **チャネルダイバーシティ** + **重み付き三辺測量** + **カルマンフィルタ**: Cantón Paterna et al. (2017) が BLE 環境で実装。チャネル間の RSSI 差異を利用してフェージングの影響を分散させる。
- **PDR（歩行者推測航法）融合**: IMU による移動推定と RSSI 観測を統合。RSSI だけでは時間的に不安定な推定を、連続的な移動モデルで安定化する。
  - Jin et al. (2023): 粒子フィルタで PDR + RSSI を融合。
  - Liu et al. (2020): iBeacon の RSSI と PDR をリアルタイム統合。
  - Ceron et al. (2020): BLE + IMU で測位と地図生成を同時に実施（SLAM 的アプローチ）。

### 3.3 AoA（Angle of Arrival）が利用可能な場合

受信機側（または送信機側）にアンテナアレイがあり、電波の到来角を推定できる場合。距離（RSSI）と角度を組み合わせることで、**送信機 1 基からでも 2D 位置を一意に決定** できる。

$$
\hat{\mathbf{p}} = \mathbf{p}_{\text{tx}} + \hat{d} \cdot \begin{pmatrix} \cos\hat{\theta} \\ \sin\hat{\theta} \end{pmatrix}
$$

- **BLE 5.1 AoA**: Qian et al. (2022) が MUSIC アルゴリズムによる角度推定を BLE 5.1 の CTE (Constant Tone Extension) で実装し性能を評価。
- **MUSIC (Multiple Signal Classification)**: 受信信号の共分散行列を固有値分解し、信号部分空間と雑音部分空間を分離して高分解能な角度推定を実現。

**精度**: 理想条件で角度誤差 3°〜5° 程度。マルチパス環境では劣化。

複数送信機すべてに AoA を適用できれば、幾何学的冗長性が高まりさらに精度が向上する。

---

## 4. 状態推定フィルタ（時系列処理）

RSSI の瞬時値はノイジーであるため、時系列フィルタによる平滑化・追跡が不可欠。Thrun et al. (2005) の *Probabilistic Robotics* が理論的基盤。

### 4.1 ベイズフィルタの一般形

状態 $\mathbf{x}_t$（位置・速度）を、制御入力 $\mathbf{u}_t$ と観測 $\mathbf{z}_t$ から逐次推定：

$$
\underbrace{p(\mathbf{x}_t | \mathbf{z}_{1:t}, \mathbf{u}_{1:t})}_{\text{事後分布}} \propto \underbrace{p(\mathbf{z}_t | \mathbf{x}_t)}_{\text{観測モデル}} \cdot \underbrace{\int p(\mathbf{x}_t | \mathbf{x}_{t-1}, \mathbf{u}_t) \, p(\mathbf{x}_{t-1} | \mathbf{z}_{1:t-1}) \, d\mathbf{x}_{t-1}}_{\text{予測（状態遷移）}}
$$

### 4.2 カルマンフィルタ（KF / EKF）

線形ガウス仮定の下で最適。Cantón Paterna et al. (2017) が BLE 測位に適用。

- **予測**: $\hat{\mathbf{x}}_{t|t-1} = F \hat{\mathbf{x}}_{t-1} + B \mathbf{u}_t$
- **更新**: $\hat{\mathbf{x}}_t = \hat{\mathbf{x}}_{t|t-1} + K_t (\mathbf{z}_t - H \hat{\mathbf{x}}_{t|t-1})$

RSSI → 距離の非線形性には **EKF（拡張カルマンフィルタ）** でヤコビアン線形化を適用。

**適用場面**: 送信機数が十分で、ノイズがおおむねガウス的な環境。

### 4.3 粒子フィルタ（パーティクルフィルタ / Sequential Monte Carlo）

非線形・非ガウスの状態空間モデルに適用可能。多数のサンプル（粒子）で事後分布を近似。

1. **予測**: 各粒子を状態遷移モデル（PDR等）で遷移。
2. **重み更新**: RSSI 観測尤度 $p(\mathbf{z}_t | \mathbf{x}_t^{(i)})$ で各粒子に重み付け。
3. **リサンプリング**: 重みに比例して粒子を再抽出。

- **Jin et al. (2023)**: BLE RSSI + PDR を粒子フィルタで融合。
- **Ceron et al. (2020)**: SLAM 的文脈での粒子フィルタ適用。

**利点**: 地図制約（壁を通過しない等）を容易に組み込める。マルチモーダル分布を表現可能。

---

## 5. RSSI ノイズへの対処技術

| 手法 | 説明 | 出典 |
|------|------|------|
| **チャネルダイバーシティ** | BLE の 3 つの広告チャネル (37/38/39) の RSSI を独立に扱い、周波数選択性フェージングの影響を分散 | Cantón Paterna et al. (2017) |
| **時間平均** | 短時間窓での移動平均・中央値フィルタ | Cinefra (2012) |
| **確率モデル化** | RSSI をガウス分布等でモデル化し、尤度計算で揺らぎを吸収 | Horus (2005) |
| **外れ値除去** | 極端な RSSI 値を棄却（MAD、IQR ベース等） | 一般的前処理 |
| **適応的パラメータ** | path-loss 指数 $n$ や $RSSI(d_0)$ を環境に応じて動的に更新 | Cinefra (2012) |

---

## 6. センサ融合アーキテクチャ

RSSI と他センサの融合パターン。PDR 融合は送信機数が少ない場合やRSSI の揺らぎが大きい環境で特に有効：

```
┌──────────────────────────────────────────────────────────┐
│               状態推定フィルタ                              │
│          (KF / EKF / 粒子フィルタ)                        │
├──────────────────────────────────────────────────────────┤
│                                                            │
│  ┌────────────────┐  ┌──────────┐  ┌──────────┐         │
│  │ RSSI観測        │  │ PDR/IMU  │  │  地図制約  │         │
│  │(複数送信機から  │  │(状態遷移)│  │ (壁・通路) │         │
│  │ の距離尤度)     │  │          │  │           │         │
│  └────────────────┘  └──────────┘  └──────────┘         │
│        ↓                  ↓              ↓                │
│    観測モデル         予測モデル       制約条件            │
│    p(z|x)           p(x_t|x_{t-1})  事後分布修正         │
└──────────────────────────────────────────────────────────┘
```

- **予測ステップ**: PDR（歩幅×方位）で次位置を予測。IMU がない場合はランダムウォークや等速モデル。
- **更新ステップ**: 各送信機からの RSSI を用いて距離尤度を計算し、位置を補正。
- **制約適用**: 地図がある場合、壁貫通粒子を棄却（粒子フィルタの場合）。

---

## 7. 複数受信機の同時推定（複数ビーコン問題）

受信機が複数台同時に存在する場合、単一受信機の手法をそのまま個別適用できるが、追加の課題と可能性が生じる：

| 課題 | 説明 |
|------|------|
| **スケーラビリティ** | 受信機数が増えるとスキャン競合や計算量が増大 |
| **データ関連付け** | どの RSSI 観測がどの受信機のものか識別する必要 |
| **協調推定** | 受信機間の相対位置情報を共有して推定精度を向上させる可能性 |
| **電波干渉** | 受信機同士が送信も行う場合、相互干渉の管理が必要 |

> survey.md の「複数ビーコン」セクションは現時点で未収集。

---

## 8. 手法比較と選定指針

| 条件 | 推奨手法 | 根拠 |
|------|----------|------|
| 送信機 ≥ 3、RSSI のみ | 重み付き三辺測量 + KF | Cantón Paterna et al. (2017) |
| 送信機 ≥ 1、AoA あり | AoA + RSSI 距離 | Qian et al. (2022) |
| RSSI + IMU あり | PDR + 粒子フィルタ + RSSI 補正 | Jin et al. (2023) |
| 送信機少数、地図あり | 粒子フィルタ + 地図制約 | Ceron et al. (2020) |
| 環境固定、事前計測可 | フィンガープリント（確率的） | Horus (2005) |

---

## 9. 精度の実績値（論文報告）

| 論文 | 構成 | 環境 | 報告精度 |
|------|------|------|----------|
| Cantón Paterna et al. (2017) | 複数 BLE 送信機 + KF | 屋内オフィス | 平均誤差 ~2 m |
| Jin et al. (2023) | BLE + PDR + 粒子フィルタ | 屋内 | 平均誤差 ~1.5 m |
| Qian et al. (2022) | BLE 5.1 AoA | 実験室 | 角度誤差 3°〜5°, 距離 1m 以内（近距離） |
| Liu et al. (2020) | iBeacon + PDR | 屋内 | 平均誤差 ~1.8 m |

> 注: RSSI 単独での精度限界はおおよそ 2〜4 m（屋内）。PDR 融合や AoA で 1〜2 m に改善される。

---

## 10. 実装上の考慮事項

### 10.1 校正（キャリブレーション）
- $RSSI(d_0)$ と $n$ は環境ごと・送信機ごとに実測で決定する必要がある。
- BLE デバイスの送信電力 (Tx Power) の個体ばらつきにも注意。

### 10.2 計算コスト
- **カルマンフィルタ**: $O(n^3)$（状態次元 $n$ が小さいため実質一定時間）。スマホでリアルタイム可。
- **粒子フィルタ**: 粒子数 $N$ に線形。$N = 100$〜$1000$ 程度でスマホ実行可能。

### 10.3 BLE 固有の制約
- 広告間隔（100 ms〜数秒）が観測レートを規定。
- チャネル 37/38/39 の周波数差による RSSI 差異。
- スマートフォンの BLE スキャン API のレート制限（Android: ~5 Hz、iOS: ~1 Hz）。

---

## 11. まとめ：RSSI 自己位置推定のパイプライン

```
[複数の BLE 送信機（位置既知）が電波を送信]
       ↓ 電波伝搬
[受信機（推定対象）が各送信機の RSSI を測定]
       ↓ 前処理（平均化、外れ値除去、チャネルダイバーシティ）
[RSSI → 距離変換 (path-loss モデル) × 送信機数]
       ↓
[位置推定アルゴリズム]
  ├─ RSSI のみ → 三辺測量 / 重み付き最小二乗
  ├─ RSSI + AoA → 角度＋距離で幾何計算（少数送信機で可）
  ├─ RSSI + PDR → IMU 予測 + RSSI 補正
  └─ フィンガープリント → RSSI ベクトルをデータベース照合
       ↓
[時系列フィルタ (KF / EKF / 粒子フィルタ)]
       ↓ （オプション）地図制約の適用
[推定位置出力]
```

---

## 参考文献

1. Bahl & Padmanabhan, "RADAR: An In-Building RF-based User Location and Tracking System," IEEE INFOCOM, 2000.
2. Youssef & Agrawala, "The Horus WLAN Location Determination System," MobiSys, 2005.
3. Rappaport, *Wireless Communications: Principles and Practice*, Prentice Hall, 1996/2002.
4. Thrun, Burgard & Fox, *Probabilistic Robotics*, MIT Press, 2005.
5. Leitch et al., "On indoor localization using WiFi, BLE, UWB, and IMU technologies," Sensors, 2023.
6. Cinefra, "An adaptive indoor positioning system based on Bluetooth Low Energy RSSI," Politecnico di Milano, 2012.
7. Cantón Paterna et al., "A BLE Indoor Positioning System with Channel Diversity, Weighted Trilateration and Kalman Filtering," Sensors, 2017.
8. Jin et al., "Real-time indoor positioning based on BLE beacons and PDR for smartphones," Applied Sciences, 2023.
9. Qian et al., "Performance analysis of BLE 5.1 new feature AoA for relative positioning," ISPRS Archives, 2022.
10. Liu et al., "Real-time indoor positioning approach using iBeacons and smartphone sensors," Applied Sciences, 2020.
11. Ceron et al., "Simultaneous indoor pedestrian localization and house mapping based on IMU and BLE beacon data," Sensors, 2020.
