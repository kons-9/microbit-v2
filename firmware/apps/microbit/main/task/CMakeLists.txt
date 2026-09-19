cmake_minimum_required(VERSION 3.16)

add_library(main_task STATIC)

target_sources(main_task PRIVATE
    src/entry_task.cpp
    src/ble_advertise_task.cpp
)

target_include_directories(main_task
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(main_task
    PRIVATE
        drivers
        fs
        log
        sysconfig
        utkernel_cpp
)
