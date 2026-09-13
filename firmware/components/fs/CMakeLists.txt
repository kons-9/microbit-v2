cmake_minimum_required(VERSION 3.16)

add_library(fs STATIC)

target_sources(fs PRIVATE
    src/fs.cpp
)

target_include_directories(fs
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(fs
    PUBLIC
        drivers
        library
        sysconfig
)

set_target_properties(fs PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/fs
)
