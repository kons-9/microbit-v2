cmake_minimum_required(VERSION 3.16)

add_library(main_crash OBJECT)

target_sources(main_crash PRIVATE
    src/crash_handler.cpp
    src/boot_check.cpp
)

target_include_directories(main_crash
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(main_crash
    PUBLIC
        sysconfig
    PRIVATE
        nrfx
)

# Reset_Handlerのラップはmain applicationにだけ適用する。
target_link_options(main_crash INTERFACE -Wl,--wrap=Reset_Handler)
