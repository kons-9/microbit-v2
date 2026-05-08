# Button コンポーネント

micro:bit v2.2 のボタンA/Bドライバ。

## ハードウェア
- Button A: P0.14 (active low, 外部 4.7K プルアップ)
- Button B: P0.23 (active low, 外部 4.7K プルアップ)
- デバウンス: ソフトウェア 50ms

## API
- `button_init()` - 初期化（GPIO入力設定）
- `button_is_pressed(id)` - 現在のボタン状態取得
- `button_wait_press(id, timeout_ms)` - 指定ボタンの押下を待つ
- `button_wait_any(timeout_ms)` - いずれかのボタン押下を待つ

## 備考
- ポーリング方式 (μT-Kernel `tk_dly_tsk` 使用)
- 外部プルアップ付きのため `NRF_GPIO_PIN_NOPULL` で設定

## ビルド
```
cmake -B build/test -DTARGET_ARCH=linux
cmake --build build/test
ctest --test-dir build/test
```
