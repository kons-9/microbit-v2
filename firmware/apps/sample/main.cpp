extern "C" {
#include <tk/tkernel.h>
#include <tm/tmonitor.h>
}

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
void tsk1(INT, void *) {
    TM_PUTSTRING((UB *)"Hello Task-1\n");
    tk_exd_tsk();
}

/* ----------------------------------------------------------
 * User Task-2
 */
void tsk2(INT, void *) {
    TM_PUTSTRING((UB *)"Hello Task-2\n");
    tk_exd_tsk();
}

const T_CTSK ctsk1 = {0, (TA_HLNG | TA_RNG3), (FP)&tsk1, 10, 1024, 0};
const T_CTSK ctsk2 = {0, (TA_HLNG | TA_RNG3), (FP)&tsk2, 11, 1024, 0};

/* ----------------------------------------------------------
 * Entry Point (runs on initial task)
 */
static inline int _main();

extern "C" int usermain(void) {
    return _main();
}

static inline int _main() {
    T_RVER rver;

    TM_PUTSTRING((UB *)"Start Sample program.\n");

    tk_ref_ver(&rver);

#if USE_TMONITOR
    tm_printf((UB *)"Make Code: %04x  Product ID: %04x\n", rver.maker, rver.prid);
    tm_printf((UB *)"Product Ver. %04x\nProduct Num. %04x %04x %04x %04x\n",
              rver.prver,
              rver.prno[0],
              rver.prno[1],
              rver.prno[2],
              rver.prno[3]);
#endif

    auto id1 = tk_cre_tsk(&ctsk1);
    tk_sta_tsk(id1, 0);

    auto id2 = tk_cre_tsk(&ctsk2);
    tk_sta_tsk(id2, 0);

    tk_slp_tsk(TMO_FEVR);

    return 0;
}
