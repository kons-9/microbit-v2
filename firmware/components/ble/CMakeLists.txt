cmake_minimum_required(VERSION 3.16)

add_library(ble STATIC)

target_sources(ble PRIVATE
    src/ble.cpp
)

target_include_directories(ble
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src/arch
)

if(NOT DEFINED TARGET_ARCH)
    message(FATAL_ERROR "TARGET_ARCH not set")
endif()

if(TARGET_ARCH STREQUAL "microbit")
    target_sources(ble PRIVATE
        src/arch/microbit/ble_microbit.cpp
    )
    target_link_libraries(ble PRIVATE nrfx)
else()
    message(FATAL_ERROR "Unknown TARGET_ARCH: ${TARGET_ARCH}")
endif()

# 出力を同一ディレクトリに統一
set_target_properties(ble PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/ble
)
