#include <tm/tmonitor.h>
#include <utkernel/task>

/*
 * Sample User Program
 *
 * origin/main のシンプルな動作確認用アプリ。
 * リンカスクリプトやカーネル起動のテストに使用する。
 */

#if USE_TMONITOR
#define TM_PUTSTRING(a) tm_putstring(a)
#else
#define TM_PUTSTRING(a)
#endif

/* ----------------------------------------------------------
 * User Task-1
 */
void tsk1(void *) {
    TM_PUTSTRING((UB *)"Hello Task-1\n");
    utkernel::task::sleep_forever();
}

/* ----------------------------------------------------------
 * User Task-2
 */
void tsk2(void *) {
    TM_PUTSTRING((UB *)"Hello Task-2\n");
    utkernel::task::sleep_forever();
}

static utkernel::task s_task1;
static utkernel::task s_task2;

/* ----------------------------------------------------------
 * Entry Point (runs on initial task)
 */

extern "C" int usermain(void) {
    TM_PUTSTRING((UB *)"Start Sample program.\n");

    utkernel::task::config task1_config;
    task1_config.priority = 10;
    task1_config.stack_size = 1024;
    s_task1.create(tsk1, task1_config);
    s_task1.start();

    utkernel::task::config task2_config;
    task2_config.priority = 11;
    task2_config.stack_size = 1024;
    s_task2.create(tsk2, task2_config);
    s_task2.start();

    utkernel::task::sleep_forever();
    return 0;
}
