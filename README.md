# BLE屋内位置推定

Bluetooth Low Energy(BLE)の受信信号強度(RSSI)を収集し、
外部AIで機械学習ベースの位置推定を行う屋内測位アプリケーション。
firmwareはwsl前提。

## build生成物
buildは各コンポーネントごとにarchiveを作成する。
BUILD_TYPEはDebug, Releaseを用意し、それぞれでディレクトリを作る。
releaseはltoやO3でビルドする。どれだけ時間がかかっても構わない。

```zsh
cmake -B build -DTARGET_ARCH=linux -DAPP_TARGET=main
cmake --build build
```

or 

```zsh
cmake -B build -DTARGET_ARCH=linux -DAPP_TARGET=factory-test
cmake --build build
```

## テスト

各コンポーネントは `build_test` ディレクトリで単体テストをビルド・実行できる（linux のみ）。

```zsh
cd firmware/components/ble
cmake -B build_test -DTARGET_ARCH=linux
cmake --build build_test
./build_test/ble/ble_test
```

```zsh
cd firmware/components/osal
cmake -B build_test -DTARGET_ARCH=linux
cmake --build build_test
./build_test/osal/osal_test
```

## フォーマット

```zsh
cd firmware
make format
```
