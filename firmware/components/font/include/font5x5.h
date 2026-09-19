#pragma once

/**
 * @file font5x5.h
 * @brief 5x5 bitmap font data.
 *
 * Each glyph is represented by five uint8_t rows. The lowest five bits of
 * each row correspond to columns 0 through 4.
 */

#include <cstdint>
#include <option.h>

namespace font5x5 {

/* ==================================================================
 * Digits 0-9
 * ================================================================== */

inline constexpr uint8_t CHAR_0[5] = {0x0E, 0x11, 0x11, 0x11, 0x0E};
inline constexpr uint8_t CHAR_1[5] = {0x04, 0x0C, 0x04, 0x04, 0x0E};
inline constexpr uint8_t CHAR_2[5] = {0x0E, 0x01, 0x0E, 0x10, 0x1F};
inline constexpr uint8_t CHAR_3[5] = {0x1E, 0x01, 0x0E, 0x01, 0x1E};
inline constexpr uint8_t CHAR_4[5] = {0x11, 0x11, 0x1F, 0x01, 0x01};
inline constexpr uint8_t CHAR_5[5] = {0x1F, 0x10, 0x1E, 0x01, 0x1E};
inline constexpr uint8_t CHAR_6[5] = {0x0E, 0x10, 0x1E, 0x11, 0x0E};
inline constexpr uint8_t CHAR_7[5] = {0x1F, 0x01, 0x02, 0x04, 0x04};
inline constexpr uint8_t CHAR_8[5] = {0x0E, 0x11, 0x0E, 0x11, 0x0E};
inline constexpr uint8_t CHAR_9[5] = {0x0E, 0x11, 0x0F, 0x01, 0x0E};

/* ==================================================================
 * Uppercase letters A-Z
 * ================================================================== */

inline constexpr uint8_t CHAR_A[5] = {0x0E, 0x11, 0x1F, 0x11, 0x11};
inline constexpr uint8_t CHAR_B[5] = {0x1E, 0x11, 0x1E, 0x11, 0x1E};
inline constexpr uint8_t CHAR_C[5] = {0x0F, 0x10, 0x10, 0x10, 0x0F};
inline constexpr uint8_t CHAR_D[5] = {0x1E, 0x11, 0x11, 0x11, 0x1E};
inline constexpr uint8_t CHAR_E[5] = {0x1F, 0x10, 0x1E, 0x10, 0x1F};
inline constexpr uint8_t CHAR_F[5] = {0x1F, 0x10, 0x1E, 0x10, 0x10};
inline constexpr uint8_t CHAR_G[5] = {0x0F, 0x10, 0x13, 0x11, 0x0F};
inline constexpr uint8_t CHAR_H[5] = {0x11, 0x11, 0x1F, 0x11, 0x11};
inline constexpr uint8_t CHAR_I[5] = {0x0E, 0x04, 0x04, 0x04, 0x0E};
inline constexpr uint8_t CHAR_J[5] = {0x07, 0x02, 0x02, 0x12, 0x0C};
inline constexpr uint8_t CHAR_K[5] = {0x11, 0x12, 0x1C, 0x12, 0x11};
inline constexpr uint8_t CHAR_L[5] = {0x10, 0x10, 0x10, 0x10, 0x1F};
inline constexpr uint8_t CHAR_M[5] = {0x11, 0x1B, 0x15, 0x11, 0x11};
inline constexpr uint8_t CHAR_N[5] = {0x11, 0x19, 0x15, 0x13, 0x11};
inline constexpr uint8_t CHAR_O[5] = {0x0E, 0x11, 0x11, 0x11, 0x0E};
inline constexpr uint8_t CHAR_P[5] = {0x1E, 0x11, 0x1E, 0x10, 0x10};
inline constexpr uint8_t CHAR_Q[5] = {0x0E, 0x11, 0x15, 0x12, 0x0D};
inline constexpr uint8_t CHAR_R[5] = {0x1E, 0x11, 0x1E, 0x12, 0x11};
inline constexpr uint8_t CHAR_S[5] = {0x0F, 0x10, 0x0E, 0x01, 0x1E};
inline constexpr uint8_t CHAR_T[5] = {0x1F, 0x04, 0x04, 0x04, 0x04};
inline constexpr uint8_t CHAR_U[5] = {0x11, 0x11, 0x11, 0x11, 0x0E};
inline constexpr uint8_t CHAR_V[5] = {0x11, 0x11, 0x11, 0x0A, 0x04};
inline constexpr uint8_t CHAR_W[5] = {0x11, 0x11, 0x15, 0x1B, 0x11};
inline constexpr uint8_t CHAR_X[5] = {0x11, 0x0A, 0x04, 0x0A, 0x11};
inline constexpr uint8_t CHAR_Y[5] = {0x11, 0x0A, 0x04, 0x04, 0x04};
inline constexpr uint8_t CHAR_Z[5] = {0x1F, 0x02, 0x04, 0x08, 0x1F};

/* ==================================================================
 * Symbols
 * ================================================================== */

inline constexpr uint8_t SYM_CHECK[5] = {0x00, 0x01, 0x02, 0x14, 0x08};
inline constexpr uint8_t SYM_CROSS[5] = {0x11, 0x0A, 0x04, 0x0A, 0x11};
inline constexpr uint8_t SYM_HEART[5] = {0x0A, 0x1F, 0x1F, 0x0E, 0x04};
inline constexpr uint8_t SYM_UP[5] = {0x04, 0x0E, 0x15, 0x04, 0x04};
inline constexpr uint8_t SYM_DOWN[5] = {0x04, 0x04, 0x15, 0x0E, 0x04};
inline constexpr uint8_t SYM_LEFT[5] = {0x04, 0x08, 0x1F, 0x08, 0x04};
inline constexpr uint8_t SYM_RIGHT[5] = {0x04, 0x02, 0x1F, 0x02, 0x04};
inline constexpr uint8_t SYM_FULL[5] = {0x1F, 0x1F, 0x1F, 0x1F, 0x1F};
inline constexpr uint8_t SYM_EMPTY[5] = {0x00, 0x00, 0x00, 0x00, 0x00};

/** Return the 5x5 bitmap for an ASCII character, or none if unsupported. */
constexpr library::Option<const uint8_t *> get(char c) noexcept {
    constexpr const uint8_t *const digits[] = {
        CHAR_0,
        CHAR_1,
        CHAR_2,
        CHAR_3,
        CHAR_4,
        CHAR_5,
        CHAR_6,
        CHAR_7,
        CHAR_8,
        CHAR_9,
    };
    constexpr const uint8_t *const alpha[] = {
        CHAR_A, CHAR_B, CHAR_C, CHAR_D, CHAR_E, CHAR_F, CHAR_G, CHAR_H, CHAR_I, CHAR_J, CHAR_K, CHAR_L, CHAR_M,
        CHAR_N, CHAR_O, CHAR_P, CHAR_Q, CHAR_R, CHAR_S, CHAR_T, CHAR_U, CHAR_V, CHAR_W, CHAR_X, CHAR_Y, CHAR_Z,
    };

    if (c >= '0' && c <= '9') {
        return library::Option<const uint8_t *>::some(digits[c - '0']);
    }
    if (c >= 'A' && c <= 'Z') {
        return library::Option<const uint8_t *>::some(alpha[c - 'A']);
    }
    if (c >= 'a' && c <= 'z') {
        return library::Option<const uint8_t *>::some(alpha[c - 'a']);
    }
    return library::Option<const uint8_t *>::none();
}

}  // namespace font5x5
