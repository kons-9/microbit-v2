# utkernel-cpp

μT-Kernel 3を隠蔽する、RAIIベースのC++抽象化層。

公開APIは`utkernel`名前空間で提供し、タスク、Mutex、Semaphore、EventFlag、
MessageBuffer、Timer、Interruptの生成・破棄をオブジェクトの寿命で管理する。

```cpp
#include <utkernel/task>
#include <utkernel/interrupt>

utkernel::task::sleep_for(100);
```

`<tk/...>`と`T_*`型は`utkernel-cpp`のアーキテクチャ実装内だけで使用する。
実機ではμT-Kernel、Linuxテストではpthread/標準POSIX APIへ接続する。

`include/c/`のヘッダは、必要なμT-Kernel Cヘッダを生成・ラップする内部互換層である。
