# OSAL (OS Abstraction Layer)

`osal::` 名前空間で RTOS 抽象化 API を提供する Header-only ライブラリ。
std 互換のものは std と同じ API 名/セマンティクスに合わせ、RTOS 固有機能は独自 API とする。

## プリミティブ一覧

| クラス | std 相当 | 用途 |
|---|---|---|
| `osal::mutex` | `std::mutex` | 排他制御 |
| `osal::lock_guard<M>` | `std::lock_guard` | RAII ロック |
| `osal::counting_semaphore<N>` | `std::counting_semaphore` | カウンティングセマフォ |
| `osal::binary_semaphore` | `std::binary_semaphore` | バイナリセマフォ |
| `osal::task` | (独自) | タスク生成・管理 |
| `osal::event_flag` | (独自) | イベントフラグ (any/all 待ち) |
| `osal::message_buffer` | (独自) | 可変長メッセージバッファ |
| `osal::cyclic_timer` | (独自) | 周期タイマ |
| `osal::oneshot_timer` | (独自) | ワンショットタイマ |

## アーキテクチャ

```
include/osal/
├── mutex              # 公開ヘッダ (クラス定義)
├── semaphore
├── task
├── eventflag
├── msgbuf
├── timer
└── arch/
    ├── linux/         # pthread / timerfd ベース実装
    └── microbit/      # μT-Kernel API ベース実装
```

公開ヘッダが末尾で `arch/<TARGET_ARCH>/*_impl.hpp` を include する。
src/ ディレクトリは不要 (完全 header-only)。

## プラットフォーム対応

| | Linux | microbit |
|---|---|---|
| mutex | `pthread_mutex_t` | `tk_cre_mtx` (TA_INHERIT) |
| semaphore | `sem_t` | `tk_cre_sem` |
| task | `pthread_create` | `tk_cre_tsk` / `tk_sta_tsk` |
| event_flag | `pthread_cond_t` + bitfield | `tk_cre_flg` |
| message_buffer | ring buffer + mutex | `tk_cre_mbf` |
| timer | `timer_create` / timerfd | `tk_cre_cyc` / `tk_cre_alm` |

## テスト

```bash
cd firmware && make test-osal   # 22 件
```

