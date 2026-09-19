TDDで開発すること。テストコードも提出すること。

## Architecture

- `apps/` 層にはロジックを入れない。初期化の呼び出し順序とタスク生成のみ。
- テスト対象となるロジックは必ず `components/` に配置する。
- `apps/` はコンポーネントを組み合わせるグルーコード（コンポジションルート）に徹する。

## Coding Style (Rust-like naming convention)

- 関数名: `snake_case` (例: `flash_fs_init`, `ble_gap_discover`)
- 型/struct/enum/class名: `PascalCase` (例: `FlashFsFileInfo`, `BLEGapEvent`)
- 定数/マクロ: `UPPER_SNAKE_CASE` (例: `PAGE_SIZE`, `LOG_E`)
- メンバ変数: `m_` prefix + snake_case (例: `m_write_offset`)
- ローカル変数/引数: `snake_case`
- ファイル名: `snake_case.cpp`, `snake_case.h`
- namespace: `snake_case` (例: `osal`)
- static内部関数も `snake_case` (例: `get_page_address`, `scan_write_offset`)

### 対象外 (外部API互換のため変更しない)
- µT-Kernel API (`tk_cre_tsk` 等)
- CMSIS/nrfx API
- Catch2 API