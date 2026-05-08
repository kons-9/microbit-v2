# 旧ビルドシステム (Makefile経由)

mtkernel_3 付属の `build_make/` Makefile をそのまま利用してカーネルをビルドし、
CMake 側では IMPORTED ライブラリとして取り込む方式のファイル群。

## ファイル

- `Makefile.kernel` — kernel のみを `libtkernel.a` としてビルドする Makefile
- `CMakeLists.txt` — 上記 Makefile を `add_custom_command` で呼び出す CMake ラッパー

## 使い方 (参考)

`CMakeLists.txt` を `kernel/CMakeLists.txt` に配置すれば、
make 経由のビルドに戻せる。

## 現行方式

現在は `kernel/cmake/` 配下のソースリスト (`sources_common.cmake`, `sources_microbit.cmake`) で
直接 CMake ビルドに移行済み。
