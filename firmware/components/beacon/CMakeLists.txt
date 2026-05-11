cmake_minimum_required(VERSION 3.16)

add_library(beacon STATIC)

target_sources(beacon PRIVATE
    src/beacon.cpp
)

target_include_directories(beacon
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../shell/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../ble/include
)

target_link_libraries(beacon
    PUBLIC
        shell
    PRIVATE
        ble
)

set_target_properties(beacon PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/beacon
)
