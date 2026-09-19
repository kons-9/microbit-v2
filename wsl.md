# WSL 開発環境セットアップ

WSL 上でファームウェアの開発・書き込みを行うための環境構築手順。

## 前提条件

- Windows 11 + WSL2 (Ubuntu)
- デバッガ（DAPLink / CMSIS-DAP 対応プローブ）が USB 接続されていること

## 1. ビルドツールのインストール（WSL 側）

```bash
sudo apt update
sudo apt install build-essential cmake gcc-arm-none-eabi wget unzip python3 python3-venv usbutils
```

## 2. カーネルのセットアップ

μT-Kernel 3 のソースが必要。パスワードは [T-Engine Forum](https://www.t-engine4u.com/info/mbit/2.html) から取得する。

```bash
cd firmware/kernel
./setup.sh
```

## 3. Python 仮想環境の作成（書き込みツール用）

pyocd による書き込みに必要な Python パッケージをインストールする。

```bash
cd firmware
make venv
```

## 4. Git フックのインストール

```bash
./hooks/install.sh
```

## 5. USB デバイスの転送（Windows → WSL）

WSL からデバッガに接続するには、Windows 側で [usbipd-win](https://github.com/dorssel/usbipd-win) を使って USB デバイスを転送する必要がある。

### Windows 側（管理者権限の PowerShell）

```powershell
# 接続されている USB デバイスの一覧を表示
usbipd list

# デバイスをバインド（初回のみ）
usbipd bind --busid <BUSID>

# WSL にアタッチ（WSL 起動後に毎回実行）
usbipd attach --wsl --busid <BUSID>
```

> `<BUSID>` は `usbipd list` で表示される対象デバイスの Bus ID（例: `2-3`）に置き換える。

### WSL 側で接続を確認

```bash
lsusb
```

デバッガが表示されれば転送成功。

## 6. udev ルールの設定（pyocd 用）

WSL 上で pyocd が USB デバイスにアクセスできるように udev ルールを追加する。

```bash
sudo tee /etc/udev/rules.d/99-pyocd.rules > /dev/null << 'EOF'
# CMSIS-DAP / DAPLink
SUBSYSTEM=="usb", ATTR{idVendor}=="0d28", MODE="0666"
EOF

sudo udevadm control --reload-rules
sudo udevadm trigger
```

> ベンダーID はデバッガの種類に合わせて変更すること。`lsusb` で確認できる。

## 7. ビルドと書き込み

```bash
cd firmware

# ビルド
make build

# フラッシュ消去（初回のみ）
make erase-flash

# 書き込み
make flash
```

詳細は [firmware/README.md](firmware/README.md) を参照。