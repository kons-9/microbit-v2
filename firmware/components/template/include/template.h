#pragma once

/**
 * @file template.h
 * @brief テンプレートコンポーネント (サンプル)
 */

#include <cstdint>

namespace tmpl {

/**
 * 2値を加算する
 * @param a  第1引数
 * @param b  第2引数
 * @return a + b
 */
int32_t add(int32_t a, int32_t b);

}  // namespace tmpl