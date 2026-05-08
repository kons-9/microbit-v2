#pragma once

#include <stdint.h>

/**
 * 5x5 LED マトリクス用フォントデータ
 *
 * 各文字は uint8_t[5] で表現。
 * bitmap[row] の bit0-4 が col0-4 に対応。
 */

#ifdef __cplusplus
extern "C" {
#endif

// ----------------------------------------------------------
// 数字 0-9
// ----------------------------------------------------------
static const uint8_t LED_CHAR_0[5] = {0x0E, 0x11, 0x11, 0x11, 0x0E};
static const uint8_t LED_CHAR_1[5] = {0x04, 0x0C, 0x04, 0x04, 0x0E};
static const uint8_t LED_CHAR_2[5] = {0x0E, 0x01, 0x0E, 0x10, 0x1F};
static const uint8_t LED_CHAR_3[5] = {0x1E, 0x01, 0x0E, 0x01, 0x1E};
static const uint8_t LED_CHAR_4[5] = {0x11, 0x11, 0x1F, 0x01, 0x01};
static const uint8_t LED_CHAR_5[5] = {0x1F, 0x10, 0x1E, 0x01, 0x1E};
static const uint8_t LED_CHAR_6[5] = {0x0E, 0x10, 0x1E, 0x11, 0x0E};
static const uint8_t LED_CHAR_7[5] = {0x1F, 0x01, 0x02, 0x04, 0x04};
static const uint8_t LED_CHAR_8[5] = {0x0E, 0x11, 0x0E, 0x11, 0x0E};
static const uint8_t LED_CHAR_9[5] = {0x0E, 0x11, 0x0F, 0x01, 0x0E};

// ----------------------------------------------------------
// 英大文字 A-Z
// ----------------------------------------------------------
static const uint8_t LED_CHAR_A[5] = {0x0E, 0x11, 0x1F, 0x11, 0x11};
static const uint8_t LED_CHAR_B[5] = {0x1E, 0x11, 0x1E, 0x11, 0x1E};
static const uint8_t LED_CHAR_C[5] = {0x0F, 0x10, 0x10, 0x10, 0x0F};
static const uint8_t LED_CHAR_D[5] = {0x1E, 0x11, 0x11, 0x11, 0x1E};
static const uint8_t LED_CHAR_E[5] = {0x1F, 0x10, 0x1E, 0x10, 0x1F};
static const uint8_t LED_CHAR_F[5] = {0x1F, 0x10, 0x1E, 0x10, 0x10};
static const uint8_t LED_CHAR_G[5] = {0x0F, 0x10, 0x13, 0x11, 0x0F};
static const uint8_t LED_CHAR_H[5] = {0x11, 0x11, 0x1F, 0x11, 0x11};
static const uint8_t LED_CHAR_I[5] = {0x0E, 0x04, 0x04, 0x04, 0x0E};
static const uint8_t LED_CHAR_J[5] = {0x07, 0x02, 0x02, 0x12, 0x0C};
static const uint8_t LED_CHAR_K[5] = {0x11, 0x12, 0x1C, 0x12, 0x11};
static const uint8_t LED_CHAR_L[5] = {0x10, 0x10, 0x10, 0x10, 0x1F};
static const uint8_t LED_CHAR_M[5] = {0x11, 0x1B, 0x15, 0x11, 0x11};
static const uint8_t LED_CHAR_N[5] = {0x11, 0x19, 0x15, 0x13, 0x11};
static const uint8_t LED_CHAR_O[5] = {0x0E, 0x11, 0x11, 0x11, 0x0E};
static const uint8_t LED_CHAR_P[5] = {0x1E, 0x11, 0x1E, 0x10, 0x10};
static const uint8_t LED_CHAR_Q[5] = {0x0E, 0x11, 0x15, 0x12, 0x0D};
static const uint8_t LED_CHAR_R[5] = {0x1E, 0x11, 0x1E, 0x12, 0x11};
static const uint8_t LED_CHAR_S[5] = {0x0F, 0x10, 0x0E, 0x01, 0x1E};
static const uint8_t LED_CHAR_T[5] = {0x1F, 0x04, 0x04, 0x04, 0x04};
static const uint8_t LED_CHAR_U[5] = {0x11, 0x11, 0x11, 0x11, 0x0E};
static const uint8_t LED_CHAR_V[5] = {0x11, 0x11, 0x11, 0x0A, 0x04};
static const uint8_t LED_CHAR_W[5] = {0x11, 0x11, 0x15, 0x1B, 0x11};
static const uint8_t LED_CHAR_X[5] = {0x11, 0x0A, 0x04, 0x0A, 0x11};
static const uint8_t LED_CHAR_Y[5] = {0x11, 0x0A, 0x04, 0x04, 0x04};
static const uint8_t LED_CHAR_Z[5] = {0x1F, 0x02, 0x04, 0x08, 0x1F};

// ----------------------------------------------------------
// 記号
// ----------------------------------------------------------
static const uint8_t LED_SYM_CHECK[5] = {0x00, 0x01, 0x02, 0x14, 0x08};  // チェックマーク ✓
static const uint8_t LED_SYM_CROSS[5] = {0x11, 0x0A, 0x04, 0x0A, 0x11};  // ×マーク
static const uint8_t LED_SYM_HEART[5] = {0x0A, 0x1F, 0x1F, 0x0E, 0x04};  // ハート ♥
static const uint8_t LED_SYM_UP[5] = {0x04, 0x0E, 0x15, 0x04, 0x04};     // ↑
static const uint8_t LED_SYM_DOWN[5] = {0x04, 0x04, 0x15, 0x0E, 0x04};   // ↓
static const uint8_t LED_SYM_LEFT[5] = {0x04, 0x08, 0x1F, 0x08, 0x04};   // ←
static const uint8_t LED_SYM_RIGHT[5] = {0x04, 0x02, 0x1F, 0x02, 0x04};  // →
static const uint8_t LED_SYM_FULL[5] = {0x1F, 0x1F, 0x1F, 0x1F, 0x1F};   // 全点灯
static const uint8_t LED_SYM_EMPTY[5] = {0x00, 0x00, 0x00, 0x00, 0x00};  // 全消灯

// ----------------------------------------------------------
// ルックアップテーブル
// ----------------------------------------------------------

/** ASCII文字からフォントパターンを取得 (未定義文字はNULLを返す) */
static inline const uint8_t *led_font_get(char c) {
    static const uint8_t *const digits[] = {
        LED_CHAR_0,
        LED_CHAR_1,
        LED_CHAR_2,
        LED_CHAR_3,
        LED_CHAR_4,
        LED_CHAR_5,
        LED_CHAR_6,
        LED_CHAR_7,
        LED_CHAR_8,
        LED_CHAR_9,
    };
    static const uint8_t *const alpha[] = {
        LED_CHAR_A, LED_CHAR_B, LED_CHAR_C, LED_CHAR_D, LED_CHAR_E, LED_CHAR_F, LED_CHAR_G, LED_CHAR_H, LED_CHAR_I,
        LED_CHAR_J, LED_CHAR_K, LED_CHAR_L, LED_CHAR_M, LED_CHAR_N, LED_CHAR_O, LED_CHAR_P, LED_CHAR_Q, LED_CHAR_R,
        LED_CHAR_S, LED_CHAR_T, LED_CHAR_U, LED_CHAR_V, LED_CHAR_W, LED_CHAR_X, LED_CHAR_Y, LED_CHAR_Z,
    };

    if (c >= '0' && c <= '9')
        return digits[c - '0'];
    if (c >= 'A' && c <= 'Z')
        return alpha[c - 'A'];
    if (c >= 'a' && c <= 'z')
        return alpha[c - 'a'];
    return (void *)0;
}

#ifdef __cplusplus
}
#endif
