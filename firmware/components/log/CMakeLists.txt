cmake_minimum_required(VERSION 3.16)

add_library(log STATIC
    src/log.cpp
)

target_include_directories(log
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(log
    PUBLIC
        uart
)

set_target_properties(log PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/log
)
