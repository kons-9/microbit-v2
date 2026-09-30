# data/ — 実測データ格納ディレクトリ

別担当が取得した RSSI 実測データをここに配置する。

## 必要ファイル

### 1. RSSI 時系列データ (`rssi_log.csv`)

```csv
timestamp_ms,anchor_id,rssi_dbm
0,anchor_1,-45.2
0,anchor_2,-52.1
0,anchor_3,-61.8
0,anchor_4,-58.3
100,anchor_1,-44.8
100,anchor_2,-53.5
100,anchor_3,-60.2
100,anchor_4,-57.9
...
```

| カラム | 型 | 説明 |
|--------|-----|------|
| `timestamp_ms` | int | 計測開始からの経過時刻 [ms] |
| `anchor_id` | str | アンカー識別子（anchor_config.csv と一致させる） |
| `rssi_dbm` | float | 受信信号強度 [dBm] |

### 2. アンカー配置情報 (`anchor_config.csv`)

```csv
anchor_id,x_m,y_m
anchor_1,0.0,0.0
anchor_2,10.0,0.0
anchor_3,10.0,10.0
anchor_4,0.0,10.0
```

| カラム | 型 | 説明 |
|--------|-----|------|
| `anchor_id` | str | アンカー識別子 |
| `x_m` | float | X座標 [m] |
| `y_m` | float | Y座標 [m] |

### 3. （任意）真位置データ (`ground_truth.csv`)

精度検証用。実測時に受信機の真の位置が既知の場合に記録する。

```csv
timestamp_ms,x_m,y_m
0,3.0,4.0
100,3.1,4.0
...
```

## 使用方法

```python
from sim.observation import RecordedSource

source = RecordedSource("sim/data/rssi_log.csv")
obs = source.get_observations(0)  # 最初の時刻の観測値
```
