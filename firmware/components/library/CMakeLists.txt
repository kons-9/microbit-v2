cmake_minimum_required(VERSION 3.16)

add_library(library INTERFACE)

target_include_directories(library
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)
