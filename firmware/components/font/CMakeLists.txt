cmake_minimum_required(VERSION 3.16)

add_library(font INTERFACE)

target_link_libraries(font
    INTERFACE library
)

target_include_directories(font
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)
