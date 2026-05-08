# Log コンポーネント

2つのログ機構を提供。

## flash_log — バイナリログ

構造体直列化ベースのバイナリログ。flash_fs の Stream ファイルに記録し、`tools/decode.py` でデコード。

### レコード構造

```
[RecordHeader (8B)] [Payload (可変)]
```

RecordHeader: version(1) + type(1) + size(1) + reserved(1) + timestamp_ms(2) + padding(2)

### エントリ型

| Type | 構造体 | サイズ | 内容 |
|---|---|---|---|
| Crash | `CrashEntry` | 48B | fault_type, pc, lr, sp, stack_dump[8] |
| Event | `EventEntry` | 8B | event_id, param |
| Ble | `BleEntry` | 8B | addr[6], rssi, event_type |
| Ota | `OtaEntry` | 8B | state, progress, bytes |

### API

```cpp
#include "flash_log.hpp"

flash_log::EventEntry ev{};
ev.event_id = 42;
ev.param = 0xDEADBEEF;
flash_log::write(ev);  // flash_fs に追記
```

### デコード

```bash
cd components/log/tools
python3 decode.py < raw_dump.bin
```

## log — テキストログ (C API)

UART にレベル付きテキストログを出力。

### マクロ

```c
#include "log.h"
LOG_E("error: %d", code);   // ERROR レベル
LOG_W("warning");            // WARN
LOG_I("info: %s", msg);     // INFO
LOG_D("debug val=%d", v);   // DEBUG
```

## テスト

```bash
cd firmware && make test-flash_log                              # C++ テスト (4件)
cd firmware/components/log/tools && python3 -m pytest test_decode.py -v  # Python テスト (16件)
```
