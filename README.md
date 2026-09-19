# BLE屋内位置推定

Bluetooth Low Energy (BLE) の受信信号強度 (RSSI) を収集し、屋内位置推定に利用するシステムです。
micro:bit v2.2 (nRF52833) 上で μT-Kernel 3 とファームウェアを動かします。

## クイックスタート

詳細な環境構築は [wsl.md](wsl.md)、ファームウェア固有の情報は [firmware/README.md](firmware/README.md) を参照してください。

```bash
# μT-Kernel 3 とサブモジュールを準備
git submodule update --init --recursive
cd firmware/kernel
./setup.sh

# メインアプリをビルド
cd ..
make build

# Linux向けユニットテストを実行
make test
```

micro:bitへの書き込みには、別途 pyocd とデバッガのUSB接続が必要です。

```bash
cd firmware
make flash
```

## 主なMakeターゲット

| コマンド | 内容 |
|---|---|
| `make build` | メインアプリをビルド |
| `make build-recovery` | リカバリー用ファームウェアをビルド |
| `make build-sample` | サンプルアプリをビルド |
| `make build-integration-test` | 実機向け統合テストをビルド |
| `make build-all` | すべてのファームウェアをビルド |
| `make test` | Linux向けユニットテストを実行 |
| `make test-<component>` | コンポーネント単位でテストを実行 |
| `make format` | clang-format-18でソースを整形 |
| `make flash` | メインアプリを書き込み |
| `make flash-recovery` | リカバリー用ファームウェアを書き込み |
| `make clean` | ビルド成果物を削除 |

デバッグビルドは `IS_DEBUG=1 make build` で作成できます。

## フォーマットとCI

ローカルでソースを整形するには、`clang-format-18` をインストールして以下を実行します。

```bash
cd firmware
make format
```

GitHub Actionsでは、`kernel`、`third_party`、ビルド生成物を除くC/C++ソースに対して、整形差分がないことをチェックします。

## ディレクトリ構成

```text
ble-locator/
├── firmware/       ファームウェア本体 (C/C++20, μT-Kernel 3)
│   ├── apps/       micro:bit向けアプリケーション
│   ├── components/ 共通ロジックとデバイスドライバ
│   ├── kernel/     μT-Kernel 3
│   ├── linker/     リンカスクリプト
│   └── test/       Linux向けユニットテスト
├── hooks/          Gitフック
├── wsl.md          WSLセットアップ手順
└── .github/        CI設定
```
