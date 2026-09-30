# BLE RSSI 推定サーバー

ESP32-C3 ScannerからBLE広告をHTTPで受け取り、4アンカー分のRSSIが500 ms以内に揃った時点で位置を推定します。推定器は `sim/estimators/enhanced.py` の Enhanced AI+PF を使用します。

## 起動

`--anchor` はManufacturer Dataに設定した6バイトのアンカーIDと対応させます。

```powershell
cd C:\Users\0108412435\Documents\private\TRONcompetition\RSSI
python -m pip install -r requirements.txt
python server.py `
  --anchor anchor_1=00:00:00:00:00:01 `
  --anchor anchor_2=00:00:00:00:00:02 `
  --anchor anchor_3=00:00:00:00:00:03 `
  --anchor anchor_4=00:00:00:00:00:04
```

既定の座標は次の順序です。

```text
anchor_1=(0,0)       anchor_2=(10,0)
anchor_4=(0,10)      anchor_3=(10,10)
```

## HTTP API

ESP32-C3 Scannerからは、既存コードの形式で `POST /ble` に送信します。

```json
{
  "address": "aa:bb:cc:dd:ee:ff",
  "rssi": -55,
  "data": "1409626c652d6c6f6361746f722d6573703332633309ff3412000000000001"
}
```

`data` 内の `ff 34 12` に続く6バイトをアンカーIDとして解釈します。4アンカーが揃うまでは推定値を返さず、揃ったPOSTのレスポンスに次の形式で推定値を含めます。

```json
{
  "accepted": true,
  "calibrated": false,
  "estimate": {"x": 4.8, "y": 5.2, "timestamp_ms": 123456},
  "observations": {"anchor_1": -55, "anchor_2": -60}
}
```

```text
GET /health    サーバー稼働確認
GET /estimate  最新RSSIと推定位置
GET /status    /estimateの別名
```

## 校正

校正なしではMLP補正をゼロにした物理モデルで動作します。実機では既知位置で各アンカーから3サンプル以上取得し、以下のJSONを指定してください。

```json
{
  "samples": [
    {
      "position": [2, 2],
      "observations": [
        {"anchor_1": -48, "anchor_2": -61, "anchor_3": -70, "anchor_4": -57},
        {"anchor_1": -49, "anchor_2": -60, "anchor_3": -71, "anchor_4": -58},
        {"anchor_1": -47, "anchor_2": -62, "anchor_3": -70, "anchor_4": -57}
      ]
    }
  ]
}
```

```powershell
python server.py ... --calibration calibration.json
```

実機校正では、10 m x 10 mの3 x 3グリッドなど複数位置を使用してください。