#!/bin/bash

# Windows側のゲートウェイIPアドレスを自動取得
WIN_IP=$(ip route show | grep default | awk '{print $3}')
BUSID="4-7"
USBIP_BIN="/var/run/usbipd-win/usbip"

if [ -z "$WIN_IP" ]; then
    echo "エラー: WindowsのIPアドレスを取得できませんでした。"
    exit 1
fi

echo "Windows (${WIN_IP}) から USBデバイス (${BUSID}) をアタッチ中..."

# 必要なモジュールを手動ロード
sudo modprobe vhci-hcd 2>/dev/null

# Windows側のusbipdに接続
sudo $USBIP_BIN attach --remote="$WIN_IP" --busid="$BUSID"

# 結果確認
echo "現在のUSB接続状態:"
lsusb