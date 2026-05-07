cmake_minimum_required(VERSION 3.16)

# ==== ビルド設定 ====
set(TARGET_ARCH "microbit" CACHE STRING "Target architecture")
set_property(CACHE TARGET_ARCH PROPERTY STRINGS linux microbit)

set(APP_TARGET "main" CACHE STRING "Application target")
set_property(CACHE APP_TARGET PROPERTY STRINGS main factory-test)

option(ENABLE_LOG "log enable" ON)

# ==== ツールチェイン ====
if(TARGET_ARCH STREQUAL "microbit")
    include(cmake/toolchain.cmake)
endif()

project(ble_locator LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ==== アーキテクチャ定義 ====
if(TARGET_ARCH STREQUAL "linux")
    add_compile_definitions(ARCH_linux)
elseif(TARGET_ARCH STREQUAL "microbit")
    add_compile_definitions(ARCH_microbit BOARD_microbit)
else()
    message(FATAL_ERROR "Unknown TARGET_ARCH=${TARGET_ARCH}")
endif()

# ==== サブディレクトリ ====
add_subdirectory(kernel)
add_subdirectory(components)

# ==== アプリケーション選択 ====
if(APP_TARGET STREQUAL "main")
    add_subdirectory(apps/main)
elseif(APP_TARGET STREQUAL "factory-test")
    add_subdirectory(apps/factory-test)
else()
    message(FATAL_ERROR "Unknown APP_TARGET=${APP_TARGET}")
endif()

# ==== リンカスクリプト（microbit のみ） ====
if(TARGET_ARCH STREQUAL "microbit")
    set(LINKER_SCRIPT
        ${CMAKE_SOURCE_DIR}/kernel/mtkernel_3/etc/linker/${TARGET_ARCH}/tkernel_map.ld
    )
    target_link_options(app_main.elf PRIVATE
        -T${LINKER_SCRIPT}
        -Wl,-Map=app_main.map
    )
endif()