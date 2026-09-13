cmake_minimum_required(VERSION 3.16)

add_library(io INTERFACE)

target_include_directories(io
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)
