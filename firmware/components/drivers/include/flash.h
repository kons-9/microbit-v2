#pragma once

#include <cstddef>
#include <cstdint>

namespace drivers {

/**
 * @brief 内蔵 Flash ドライバ
 *
 * API は共通ですが、実装は TARGET_ARCH に応じて CMake で選択される
 * flash_*.cpp が提供する。
 */
class Flash {
  public:
    void page_erase(uint32_t address);
    void write(uint32_t address, const void *data, size_t size);
    void read(uint32_t address, void *data, size_t size);
};

}  // namespace drivers
