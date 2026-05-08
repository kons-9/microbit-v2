cmake_minimum_required(VERSION 3.16)

add_library(button STATIC)

target_sources(button PRIVATE
    src/button.cpp
)

target_include_directories(button
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src/arch
)

if(NOT DEFINED TARGET_ARCH)
    message(FATAL_ERROR "TARGET_ARCH not set")
endif()

if(TARGET_ARCH STREQUAL "microbit")
    target_sources(button PRIVATE
        src/arch/microbit/button_microbit.cpp
    )
    target_link_libraries(button PRIVATE nrfx kernel)
else()
    message(FATAL_ERROR "Unknown TARGET_ARCH: ${TARGET_ARCH}")
endif()
