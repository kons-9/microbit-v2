# utkernel-cpp

μT-Kernel 3 の C API を C++ から使うためのラッパーヘッダ。

`gen_wrappers.sh` でカーネルヘッダから自動生成。

## 使い方

```cpp
#include <c/tk/tkernel.h>
```

C++ ソースから `extern "C"` なしで μT-Kernel API を呼べる。