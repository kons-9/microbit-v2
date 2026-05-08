cmake_minimum_required(VERSION 3.16)

# Header-only (INTERFACE) library — ソースファイルなし
add_library(sysconfig INTERFACE)

target_include_directories(sysconfig
    INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include
)
