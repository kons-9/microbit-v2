---
applyTo: "firmware/kernel/**"
---

# μT-Kernel 3 カーネル作業時の注意事項

## ソース管理

`firmware/kernel/mtkernel_3/` は外部ZIPアーカイブから展開されるサードパーティコードであり、
Git管理外（.gitignore対象）です。直接編集してもsetup.sh再実行時に上書きされます。

## コンフィグ変更の方法

`config/config.h` 等のカーネル設定を変更する場合は、
**`firmware/kernel/setup.sh` のパッチセクションに `sed` コマンドを追加**してください。

```bash
# 例: USE_TMONITOR を無効化
sed -i 's/^#define[[:space:]]*USE_TMONITOR[[:space:]]*([[:digit:]])/#define\tUSE_TMONITOR\t\t(0)/' "$CONFIG_H"
```

直接 config.h を編集するだけでは再現性がありません。

## UART0 の排他利用

- `USE_TMONITOR` は `0` に設定済み（setup.shでパッチ）
- カーネルの T-Monitor (`tm_printf` 等) と `firmware/components/drivers/uart` は
  同じ UART0 ペリフェラルを異なるモードで使うため、同時利用不可
- アプリ側で `tm_printf` を使う場合は `#if USE_TMONITOR` ガードで囲むこと
