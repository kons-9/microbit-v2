#!/usr/bin/env bash
# μT-Kernel 3 (micro:bit向け) セットアップスクリプト
#
# 使い方:
#   ./setup.sh              # パスワードを対話入力
#   ./setup.sh <PASSWORD>   # パスワードを引数で指定
#
# パスワードは https://www.t-engine4u.com/info/mbit/2.html を参照

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
ZIP_URL="https://www.personal-media.co.jp/book/tw/data/362_mbit_mtk3.zip"
ZIP_FILE="362_mbit_mtk3.zip"
EXPECTED_DIR="${SCRIPT_DIR}/mtkernel_3"

# --- 既にセットアップ済みかチェック ---
if [[ -d "$EXPECTED_DIR" ]]; then
    echo "mtkernel_3/ は既に存在します。再セットアップする場合は先に削除してください。"
    exit 0
fi

# --- パスワード取得 ---
if [[ $# -ge 1 ]]; then
    PASSWORD="$1"
else
    echo "パスワードを入力してください (https://www.t-engine4u.com/info/mbit/2.html 参照):"
    read -rs PASSWORD
    echo
fi

if [[ -z "$PASSWORD" ]]; then
    echo "エラー: パスワードが空です。" >&2
    exit 1
fi

cd "$SCRIPT_DIR"

# --- ダウンロード ---
if [[ ! -f "$ZIP_FILE" ]]; then
    echo "ダウンロード中: $ZIP_URL"
    wget -q --show-progress "$ZIP_URL"
else
    echo "ZIPファイルは既にダウンロード済みです: $ZIP_FILE"
fi

# --- 展開 ---
echo "展開中: $ZIP_FILE"
if ! unzip -P "$PASSWORD" -q "$ZIP_FILE"; then
    echo "エラー: 展開に失敗しました。パスワードを確認してください。" >&2
    rm -rf "$ZIP_FILE"
    rm -rf "$EXPECTED_DIR"
    exit 1
fi

# --- 検証 ---
if [[ ! -d "$EXPECTED_DIR" ]]; then
    echo "警告: mtkernel_3/ が見つかりません。ZIP内のディレクトリ構造を確認してください。" >&2
    ls -la "$SCRIPT_DIR"
    exit 1
fi

# --- ZIPファイル削除 ---
rm -f "$ZIP_FILE"

# --- プロジェクト固有のコンフィグ適用 ---
CONFIG_H="${EXPECTED_DIR}/config/config.h"
echo "コンフィグをパッチ中: $CONFIG_H"

# USE_TMONITOR を無効化 (UART0 をアプリ側で排他利用するため)
sed -i 's/^#define[[:space:]]*USE_TMONITOR[[:space:]]*([[:digit:]])/#define\tUSE_TMONITOR\t\t(0)/' "$CONFIG_H"

echo "セットアップ完了: $EXPECTED_DIR"
