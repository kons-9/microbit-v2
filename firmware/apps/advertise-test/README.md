# advertise-test

uT-Kernel上でnRF52833のRADIOを直接操作し、Advertisingだけを行う確認用アプリです。

## ビルドと書き込み

```bash
cd firmware
make build-advertise-test
make flash-advertise-test
```

以前にSoftDeviceを含むファームウェアを書き込んでいる個体を初めて切り替える場合は、
全消去を伴う次のターゲットを1回だけ使用します。

```bash
make flash-advertise-test-recover
```

この操作では既存のファームウェア、ログ、設定が消去されます。

SoftDeviceは使用しません。起動後、`BLE_TEST`という名前で100ms間隔の
`ADV_NONCONN_IND`を送信します。

WindowsのBluetooth LEスキャナ、またはnRF Sniffer + Wiresharkで確認します。
