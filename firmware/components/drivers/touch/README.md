# Touch コンポーネント

micro:bit v2.2 のタッチロゴドライバ。

## ハードウェア
- FACE_TOUCH: P1.04
- 方式: 静電容量式 (10Mohm プルアップによるRC時定数検出)
- タッチ時: LOW / 非タッチ時: HIGH

## API
- `touch_init()` - 初期化（GPIO入力設定）
- `touch_is_touched()` - 現在のタッチ状態取得
- `touch_wait(timeout_ms)` - タッチ検出を待つ

## 備考
- ポーリング方式 (μT-Kernel `tk_dly_tsk` 使用)
- 外部10Mohmプルアップにより非タッチ時HIGH

## ビルド
```
cmake -B build/test -DTARGET_ARCH=linux
cmake --build build/test
ctest --test-dir build/test
```
