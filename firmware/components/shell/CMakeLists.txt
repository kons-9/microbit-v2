cmake_minimum_required(VERSION 3.16)

add_library(shell STATIC)

target_sources(shell PRIVATE
    src/shell.cpp
)

target_include_directories(shell
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../drivers/uart/include
)

target_link_libraries(shell
    PUBLIC
        flash_fs
)

set_target_properties(shell PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/shell
)
