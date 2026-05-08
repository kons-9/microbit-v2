cmake_minimum_required(VERSION 3.16)

add_library(signal STATIC)

target_sources(signal PRIVATE
    src/signal.cpp
)

target_include_directories(signal
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

if(NOT DEFINED TARGET_ARCH)
    message(FATAL_ERROR "TARGET_ARCH not set")
endif()

set_target_properties(signal PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/signal
)

# テストは firmware/test/CMakeLists.txt で統合管理
