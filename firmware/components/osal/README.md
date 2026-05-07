# OSAL (OS Abstraction Layer) 実装プラン

## 概要
`osal::` 名前空間でRTOS抽象化APIを提供する。  
std互換可能なものはstdと同じAPI名/セマンティクスに合わせ、RTOS固有機能は独自APIとする。

## ディレクトリ構成
```
components/osal/
├── CMakeLists.txt
├── README.md              ← このファイル
├── include/
│   └── osal/
│       ├── mutex          ← osal::mutex, osal::lock_guard (公開API)
│       ├── semaphore      ← osal::counting_semaphore
│       ├── task           ← osal::task (独自)
│       ├── eventflag      ← osal::event_flag (独自)
│       ├── msgbuf         ← osal::message_buffer (独自)
│       ├── timer          ← osal::cyclic_timer, osal::oneshot_timer (独自)
│       └── arch/
│           ├── linux/
│           │   ├── mutex_impl.hpp
│           │   ├── semaphore_impl.hpp
│           │   ├── task_impl.hpp
│           │   ├── eventflag_impl.hpp
│           │   ├── msgbuf_impl.hpp
│           │   └── timer_impl.hpp
│           └── microbit/
│               ├── mutex_impl.hpp
│               ├── semaphore_impl.hpp
│               ├── task_impl.hpp
│               ├── eventflag_impl.hpp
│               ├── msgbuf_impl.hpp
│               └── timer_impl.hpp
└── test/
    └── linux/osal_test.cpp
```

**Header-onlyライブラリ**。src/は不要。
公開ヘッダー（拡張子なし）がクラス定義を持ち、末尾で`arch/<ARCH>/*_impl.hpp`をincludeする。

## API設計

### 1. `osal::mutex` — std::mutex互換
```cpp
namespace osal {
class mutex {
public:
    mutex();
    ~mutex();
    void lock();
    bool try_lock();
    void unlock();
};

// std::lock_guard互換
template<class Mutex>
using lock_guard = std::lock_guard<Mutex>;  // そのまま使える
}
```
| | Linux | microbit |
|---|---|---|
| 実装 | `pthread_mutex_t` | `tk_cre_mtx` (TA_INHERIT) |

### 2. `osal::counting_semaphore` — std::counting_semaphore互換
```cpp
namespace osal {
template<int LeastMaxValue = INT_MAX>
class counting_semaphore {
public:
    explicit counting_semaphore(int initial);
    ~counting_semaphore();
    void acquire();                  // ブロック待ち
    bool try_acquire();              // ポーリング
    bool try_acquire_for(uint32_t timeout_ms);  // タイムアウト付き
    void release(int update = 1);
};

using binary_semaphore = counting_semaphore<1>;
}
```
| | Linux | microbit |
|---|---|---|
| 実装 | `sem_t` | `tk_cre_sem` |

### 3. `osal::task` — 独自（std::threadとはセマンティクスが異なる）
```cpp
namespace osal {
class task {
public:
    struct config {
        const char* name = nullptr;
        int         priority = 10;     // 低い値 = 高優先度
        size_t      stack_size = 1024;
        void*       param = nullptr;
    };

    using entry_t = void(*)(void* param);

    task() = default;
    task(entry_t entry, const config& cfg);
    ~task();

    bool create(entry_t entry, const config& cfg);
    bool start();
    bool terminate();
    bool suspend();
    bool resume();

    // std::threadに近い静的メソッド
    static void sleep_for(uint32_t ms);
    static void yield();
    static uint32_t current_id();

    bool joinable() const;
};
}
```
| | Linux | microbit |
|---|---|---|
| 実装 | `pthread_create` | `tk_cre_tsk` / `tk_sta_tsk` |

**std::threadとの違い:**
- create/start分離（RTOSではタスク生成と起動が別）
- 優先度・スタックサイズ指定
- suspend/resume（RTOSのみ意味がある、Linux側はcond_waitで模擬）

### 4. `osal::event_flag` — 独自（std相当なし）
```cpp
namespace osal {
class event_flag {
public:
    enum wait_mode : uint32_t {
        any = 0x01,  // OR待ち
        all = 0x00,  // AND待ち
    };

    event_flag(uint32_t initial = 0);
    ~event_flag();

    void set(uint32_t bits);
    void clear(uint32_t bits);
    uint32_t wait(uint32_t pattern, wait_mode mode, uint32_t timeout_ms = UINT32_MAX);
};
}
```
| | Linux | microbit |
|---|---|---|
| 実装 | `pthread_cond_t` + bitfield | `tk_cre_flg` |

### 5. `osal::message_buffer` — 独自（std相当なし）
```cpp
namespace osal {
class message_buffer {
public:
    message_buffer(size_t buf_size, size_t max_msg_size);
    ~message_buffer();

    bool send(const void* data, size_t size, uint32_t timeout_ms = UINT32_MAX);
    size_t receive(void* data, uint32_t timeout_ms = UINT32_MAX);
};
}
```
| | Linux | microbit |
|---|---|---|
| 実装 | pipe or ring buffer + mutex | `tk_cre_mbf` |

### 6. `osal::cyclic_timer` / `osal::oneshot_timer` — 独自
```cpp
namespace osal {
class cyclic_timer {
public:
    using handler_t = void(*)(void* param);

    cyclic_timer(handler_t handler, uint32_t interval_ms, void* param = nullptr);
    ~cyclic_timer();

    void start();
    void stop();
};

class oneshot_timer {
public:
    using handler_t = void(*)(void* param);

    oneshot_timer(handler_t handler, void* param = nullptr);
    ~oneshot_timer();

    void start(uint32_t delay_ms);
    void stop();
};
}
```
| | Linux | microbit |
|---|---|---|
| 実装 | `timer_create` / timerfd | `tk_cre_cyc` / `tk_cre_alm` |

## 実装方針

1. **Header-only** — `.cpp`不要。全てインライン実装
2. **公開ヘッダーはOS非依存** — `include/osal/mutex`等にOS固有型は出さない
3. **arch分離** — 各公開ヘッダー末尾で `#include "arch/<ARCH>/*_impl.hpp"` する。CMakeの`ARCH_linux`/`ARCH_microbit`定義で切り替え
4. **エラー戦略** — `bool`返却 or assert。例外は使わない（組み込み）
5. **タイムアウト** — `uint32_t timeout_ms`統一。`UINT32_MAX`で永久待ち、`0`でポーリング
6. **include/c/** — utkernel-cppコンポーネント側のextern Cラッパーを利用。microbit実装からのカーネルincludeは透過的に行える

## 実装順序

| Phase | 内容 | テスト |
|-------|------|--------|
| 1 | `mutex` + `lock_guard` | Linux: pthread_mutexで排他テスト |
| 2 | `counting_semaphore` | Linux: producer-consumerテスト |
| 3 | `task` | Linux: 起動・sleep・優先度テスト |
| 4 | `event_flag` | Linux: set/wait/clearテスト |
| 5 | `message_buffer` | Linux: send/receiveテスト |
| 6 | `timer` | Linux: タイマー発火テスト |
| 7 | microbit実装 | ファームウェアビルド通し |

## 依存関係
```
app (main.cpp)
  └── osal
        ├── [linux]   pthread
        └── [microbit] kernel (utkernel-cppのinclude/c経由)
```
