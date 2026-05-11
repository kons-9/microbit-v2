cmake_minimum_required(VERSION 3.16)

add_library(crash STATIC)

target_include_directories(crash
    PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}/include
)

if(TARGET_ARCH STREQUAL "microbit")
    target_sources(crash PRIVATE
        src/arch/microbit/crash_handler.cpp
    )
    target_link_libraries(crash
        PUBLIC  sysconfig
        PRIVATE nrfx
    )

    if(NOT "UPDATER_MODE" IN_LIST ARCH_DEFINITIONS)
        # main app のみ: Reset_Handler をラップしてブートモード判定を挿入
        # boot_check.cpp は INTERFACE ソースとして実行ファイルに直接コンパイルされる
        target_sources(crash INTERFACE
            ${CMAKE_CURRENT_SOURCE_DIR}/src/arch/microbit/boot_check.cpp
        )
        target_link_options(crash INTERFACE -Wl,--wrap=Reset_Handler)
    endif()
else()
    message(FATAL_ERROR "Unknown TARGET_ARCH: ${TARGET_ARCH}")
endif()
