/**
 * @file main.cpp
 * @brief BLE ビーコンアプリケーション エントリポイント
 *
 * µT-Kernel 3 の usermain から呼ばれ、ユーザタスクを生成・起動する。
 *
 * タスク構成:
 *   - Shell タスク: UART からシェルコマンドを受け付ける
 *   - Beacon は beacon_init() / beacon_start() で制御
 *
 * NOTE: usermain はカーネル初期タスクのコンテキストで実行されるため、
 *       タスク停止系のシステムコールを直接発行してはいけない。
 *       usermain ではタスク生成のみ行い、アプリケーションロジックは
 *       ユーザタスク内で実行すること。
 */

#include <tk/tkernel.h>
#include <tm/tmonitor.h>

#include "beacon.h"
#include "shell.h"
#include "uart.h"
#include "flash_fs.h"

/* ==================================================================
 * ヘルパー
 * ================================================================== */

#if USE_TMONITOR
#define TM_PUT(a) tm_putstring(a)
#else
#define TM_PUT(a)
#endif

/* ==================================================================
 * Shell Task
 * ================================================================== */

static void shell_task(INT stacd, void *exinf) {
    (void)stacd;
    (void)exinf;

    for (;;) {
        shell_poll();
        tk_dly_tsk(10); /* 10ms ポーリング周期 */
    }
}

/* ==================================================================
 * Task definitions
 * ================================================================== */

static const T_CTSK s_ctsk_shell = {0,
                                    (TA_HLNG | TA_RNG3),
                                    reinterpret_cast<FP>(&shell_task),
                                    10,   /* 優先度 */
                                    2048, /* スタックサイズ */
                                    0};

/* ==================================================================
 * Entry Point
 * ================================================================== */

static int app_main();

extern "C" int usermain(void) {
    return app_main();
}

static int app_main() {
    TM_PUT(reinterpret_cast<UB *>(const_cast<char *>("BLE Beacon App\n")));

    /* UART 初期化 */
    uart_init(nullptr);

    /* Flash FS 初期化 */
    flash_fs_init();

    /* Beacon モジュール初期化 (BLE init 含む) */
    beacon_init(nullptr);

    /* Shell 初期化 (beacon コマンド登録) */
    uint8_t cmd_count = 0;
    const ShellCommand *cmds = beacon_get_shell_commands(&cmd_count);
    shell_init(cmds, cmd_count);

    /* Beacon 自動開始 */
    beacon_start();

    /* Shell タスク生成・起動 */
    auto id = tk_cre_tsk(&s_ctsk_shell);
    tk_sta_tsk(id, 0);

    tk_slp_tsk(TMO_FEVR);

    return 0;
}
