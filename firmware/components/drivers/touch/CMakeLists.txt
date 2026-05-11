cmake_minimum_required(VERSION 3.16)

add_library(touch STATIC)

target_sources(touch PRIVATE
    src/touch.cpp
)

target_include_directories(touch
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src/arch
)

if(NOT DEFINED TARGET_ARCH)
    message(FATAL_ERROR "TARGET_ARCH not set")
endif()

if(TARGET_ARCH STREQUAL "microbit")
    target_sources(touch PRIVATE
        src/arch/microbit/touch_microbit.cpp
    )
    target_link_libraries(touch PRIVATE nrfx kernel log)
else()
    message(FATAL_ERROR "Unknown TARGET_ARCH: ${TARGET_ARCH}")
endif()
