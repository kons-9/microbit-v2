# UART コンポーネント

micro:bit v2.2 の UART ドライバ。USB CDC 経由でホスト PC と通信する。

## ハードウェア

- nRF52833 UARTE0 ペリフェラル
- TX: P0.06 → USB Interface MCU → USB CDC
- RX: P1.08 ← USB Interface MCU ← USB CDC
- フロー制御なし

## API

- `uart_init(config)` - 初期化（デフォルト: 115200bps）
- `uart_write(data, len)` - データ送信
- `uart_read(buf, len, timeout)` - データ受信（ブロッキング）
- `uart_puts(str)` - 文字列送信
