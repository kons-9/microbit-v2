cmake_minimum_required(VERSION 3.16)

add_library(shell STATIC)

target_sources(shell PRIVATE
    src/shell.cpp
    src/file_commands.cpp
)

target_include_directories(shell
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(shell
    PUBLIC
        fs
        io
)

set_target_properties(shell PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/shell
)
