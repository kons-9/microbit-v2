cmake_minimum_required(VERSION 3.16)

add_library(io STATIC
    src/log.cpp
)

target_include_directories(io
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

set_target_properties(io PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/io
)
