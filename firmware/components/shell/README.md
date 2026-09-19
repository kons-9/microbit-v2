# Shell コンポーネント

UART 経由の対話シェル。デバッグ・ログ取得用。

## コマンド

| コマンド | 説明 |
|---|---|
| `help` | コマンド一覧表示 |
| `ls` | Flash FS ファイル一覧 (名前 + 使用量) |
| `cat <file>` | ファイル内容を hex dump |
| `erase <file>` | ファイル消去 |

アプリケーションから追加コマンドを登録可能 (`shell_init` の `extra_cmds` 引数)。

## API

- `shell_init(extra_cmds, num_cmds)` - シェル初期化（組み込みコマンド + 追加コマンド登録）
- `shell_poll()` - UART から 1 文字読み取り、行完成時にコマンド実行
- `shell_puts(str)` - 文字列出力
- `shell_printf(fmt, ...)` - printf 出力

## 設計

- 1 文字ずつ `shell_poll()` で処理（ノンブロッキング）
- Backspace 対応
- 行バッファ 128 bytes

## テスト

```bash
cd firmware && make test-shell   # 9 件
```
