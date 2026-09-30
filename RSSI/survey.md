# 1ビーコン

シングルビーコン（単一アンカー）構成での位置推定に有用なアルゴリズム論文を収集。
分類の考え方:
- **AoA 利用可**: アンテナアレイで到来角(AoA)が得られる → 角度 + 距離(RSSI/測距) で 1 基だけで 2D 位置を一意に決定できる。
- **AoA 利用不可**: RSSI（距離のみ）しか得られない → 1 基だけでは円周上に不定。IMU/PDR・移動・地図・粒子フィルタ等で補って位置を推定する。

---

## 推定の基礎を理解するための読書ガイド

推定の土台を学ぶ推奨順序。「全体像 → RSSI測距 → 推定アルゴリズム → フィルタ → AoA」。

> 注意: RSSI 測位の中核（電波伝搬の path-loss モデル、三辺測量、ベイズフィルタ）は
> 2000〜2005 年頃に確立済みの枯れた技術。最近(2017〜)の論文は基礎の刷新ではなく
> 「BLE/BLE 5.1 AoA という新ハードへの適用」「IMU/PDR 融合・機械学習などの組合せ」が新しいだけ。
> 基礎理解には、まず下記「0. 古典」を読み、その後に BLE 固有の応用論文を読むのが効率的。

### 0. 古典（基礎理論：まずここから）
- **RADAR: An In-Building RF-based User Location and Tracking System**（Bahl & Padmanabhan, IEEE INFOCOM 2000）
  RF(電波強度)による屋内測位の記念碑的論文。受信信号強度を事前に地図化して照合する「フィンガープリント(シーン解析)法」と、伝搬モデルによる幾何学的測位の両方を初めて体系化。以後のほぼ全ての RSSI 測位研究の出発点。被引用 1 万超。
  https://www.microsoft.com/en-us/research/publication/radar-an-in-building-rf-based-user-location-and-tracking-system/
- **The Horus WLAN Location Determination System**（Youssef & Agrawala, MobiSys 2005）
  RSSI の揺らぎを確率分布としてモデル化し、最尤推定で位置を当てる「確率的フィンガープリント」の定番。RADAR の決定論的手法を統計的に拡張し、RSSI が本質的にノイジーであることへの標準的対処を確立。
  https://www.cs.umd.edu/~moustafa/papers/horus_usenix.pdf
- **Wireless Communications: Principles and Practice**（T. S. Rappaport, 教科書, 1996/2002）
  RSSI→距離変換の根拠である log-distance path-loss モデル（受信電力が距離の対数に比例して減衰し、対数正規シャドウイングが乗る）の出典。距離推定の式がどこから来るのかを理解する一次資料。
- **Probabilistic Robotics**（Thrun, Burgard & Fox, 教科書, 2005）
  Bayes フィルタ → Kalman フィルタ → 粒子フィルタを統一的に導出する状態推定の定番教科書。本サーベイの測位手法で使われるフィルタ理論そのものを数式から理解するための土台。

### 1. 全体像をつかむ（サーベイ）
- **On indoor localization using WiFi, BLE, UWB, and IMU technologies**（Leitch et al., Sensors 2023）
  測位技術の全体俯瞰。RSSI/AoA/ToA、測距 vs フィンガープリント、単一/複数ビーコンの違いを把握できる出発点。
  https://www.mdpi.com/1424-8220/23/20/8598

### 2. RSSI → 距離 の原理（測距の基礎）
- **An adaptive indoor positioning system based on Bluetooth Low Energy RSSI**（Cinefra, 修士論文 2012）
  RSSI 測位を一から扱う学位論文。経路損失(log-distance path-loss)モデル・校正・誤差要因が丁寧。基礎固めに最適。
  https://www.politesi.polimi.it/handle/10589/92284

### 3. 距離 → 位置 の推定アルゴリズム（核心）
- **A Bluetooth Low Energy Indoor Positioning System with Channel Diversity, Weighted Trilateration and Kalman Filtering**（Cantón Paterna et al., Sensors 2017）
  三辺測量(trilateration) + 重み付き + Kalman フィルタという「距離から座標を出す」古典的王道。推定の中核手法を一通り学べる。
  https://www.mdpi.com/1424-8220/17/12/2927

### 4. 時系列での平滑化・追跡（フィルタ理論）
- **Real-time indoor positioning based on BLE beacons and pedestrian dead reckoning for smartphones**（Jin et al., Applied Sciences 2023）
  RSSI と PDR を粒子フィルタ枠組みで融合。ノイズに対する逐次推定(Bayesフィルタ)の実践例。
  https://www.mdpi.com/2076-3417/13/7/4415

### 5. AoA（角度）を理解する
- **Performance analysis of BLE 5.1 new feature angle of arrival for relative positioning**（Qian et al., ISPRS 2022）
  BLE 5.1 の AoA 機構と MUSIC アルゴリズムの実装詳細。角度推定の入門。
  https://isprs-archives.copernicus.org/articles/XLVI-3-W1-2022/155/2022/

> 補足: Kalman/粒子フィルタの数理そのものを深掘りするなら、状態推定の定番教科書
> *Probabilistic Robotics*（Thrun, Burgard & Fox）の Bayes filter / Kalman / particle filter の章が有用。

---

## AoA 利用可

- **BLE を用いた屋内位置推定における誤差低減の検討**（FIT2024）
  https://www.ieice.org/publications/conference-FIT-DVDs/FIT2024/data/html/program/pdf/M-019.pdf

- **Performance analysis of BLE 5.1 new feature angle of arrival for relative positioning**（Qian et al., ISPRS Archives, 2022）
  BLE 5.1 の AoA 機能の性能解析。MUSIC ベースの実装詳細。
  https://isprs-archives.copernicus.org/articles/XLVI-3-W1-2022/155/2022/

## AoA 利用不可

- **A Bluetooth Low Energy Indoor Positioning System with Channel Diversity, Weighted Trilateration and Kalman Filtering**（Cantón Paterna et al., Sensors, 2017）
  チャネルダイバーシティ + 重み付き三辺測量 + Kalman フィルタ。RSSI 揺らぎ低減の基礎手法。
  https://www.mdpi.com/1424-8220/17/12/2927

- **Real-time indoor positioning based on BLE beacons and pedestrian dead reckoning for smartphones**（Jin et al., Applied Sciences, 2023）
  BLE と PDR を粒子フィルタ枠組みで融合。スマホ単体で実時間測位。
  https://www.mdpi.com/2076-3417/13/7/4415

- **Real-time indoor positioning approach using iBeacons and smartphone sensors**（Liu et al., Applied Sciences, 2020）
  PDR で得た位置と各ビーコンの RSSI(距離)寄与を統合。
  https://www.mdpi.com/2076-3417/10/6/2003

- **Simultaneous indoor pedestrian localization and house mapping based on IMU and BLE beacon data**（Ceron et al., Sensors, 2020）
  少数 BLE ビーコン + IMU で測位と地図生成を同時に実施。
  https://www.mdpi.com/1424-8220/20/17/4742

- **An adaptive indoor positioning system based on Bluetooth Low Energy RSSI**（Cinefra, Politecnico di Milano, 2012）
  RSSI ベース測位の適応化。アンカー構成・経路損失モデルの基礎。
  https://www.politesi.polimi.it/handle/10589/92284

- **On indoor localization using WiFi, BLE, UWB, and IMU technologies**（Leitch et al., Sensors, 2023）
  各技術のサーベイ。「単一ビーコン・単一測定での測位可否」に言及。手法俯瞰に有用。
  https://www.mdpi.com/1424-8220/23/20/8598

# 複数ビーコン
