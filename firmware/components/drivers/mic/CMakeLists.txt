cmake_minimum_required(VERSION 3.16)

add_library(mic STATIC)

target_sources(mic PRIVATE
    src/mic.cpp
)

target_include_directories(mic
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src/arch
)

if(NOT DEFINED TARGET_ARCH)
    message(FATAL_ERROR "TARGET_ARCH not set")
endif()

if(TARGET_ARCH STREQUAL "microbit")
    target_sources(mic PRIVATE
        src/arch/microbit/mic_microbit.cpp
    )
    target_link_libraries(mic PRIVATE nrfx)
else()
    message(FATAL_ERROR "Unknown TARGET_ARCH: ${TARGET_ARCH}")
endif()
