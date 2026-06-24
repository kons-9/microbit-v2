cmake_minimum_required(VERSION 3.16)

# ==== アプリケーション選択（これだけ指定すればよい） ====
set(APP_TARGET "main" CACHE STRING "Application target")
set_property(CACHE APP_TARGET PROPERTY STRINGS main factory-test updater integration-test ble-test)

option(ENABLE_LOG "log enable" ON)

# 外部から追加フラグを注入 (e.g. -DEXTRA_C_FLAGS="-DDEBUG_CRASH")
set(EXTRA_C_FLAGS "" CACHE STRING "Additional C compile flags")
set(EXTRA_CXX_FLAGS "" CACHE STRING "Additional C++ compile flags")

# ==== app.cmake が TARGET_ARCH, ツールチェイン, ARCH_DEFINITIONS を定義 ====
set(APP_DIR apps/${APP_TARGET})
if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${APP_DIR}/app.cmake)
    message(FATAL_ERROR "Unknown APP_TARGET=${APP_TARGET}")
endif()
include(${APP_DIR}/app.cmake)

# app.cmake 事後条件チェック
if(NOT DEFINED TARGET_ARCH)
    message(FATAL_ERROR "${APP_DIR}/app.cmake must set TARGET_ARCH")
endif()
if(NOT DEFINED ARCH_DEFINITIONS)
    message(FATAL_ERROR "${APP_DIR}/app.cmake must set ARCH_DEFINITIONS")
endif()

project(ble_locator LANGUAGES C CXX ASM)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_compile_definitions(${ARCH_DEFINITIONS})

# 外部フラグを適用
if(EXTRA_C_FLAGS)
    separate_arguments(_c_flags UNIX_COMMAND "${EXTRA_C_FLAGS}")
    add_compile_options($<$<COMPILE_LANGUAGE:C>:${_c_flags}>)
endif()
if(EXTRA_CXX_FLAGS)
    separate_arguments(_cxx_flags UNIX_COMMAND "${EXTRA_CXX_FLAGS}")
    add_compile_options($<$<COMPILE_LANGUAGE:CXX>:${_cxx_flags}>)
endif()

# ==== サブディレクトリ ====
add_subdirectory(third_party)
add_subdirectory(kernel)

# Treat warnings as errors for project code (excludes third_party/kernel)
add_compile_options(-Werror)
add_subdirectory(components)
add_subdirectory(${APP_DIR})