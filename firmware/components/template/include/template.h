#pragma once

/**
 * @file template.h
 * @brief テンプレートコンポーネント (サンプル)
 */

#include <cstdint>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * 2値を加算する
 * @param a  第1引数
 * @param b  第2引数
 * @return a + b
 */
int32_t template_add(int32_t a, int32_t b);

#ifdef __cplusplus
}
#endif