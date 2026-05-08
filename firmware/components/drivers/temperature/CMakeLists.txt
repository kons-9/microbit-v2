cmake_minimum_required(VERSION 3.16)

add_library(temperature STATIC)

target_sources(temperature PRIVATE
    src/temperature.cpp
)

target_include_directories(temperature
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src/arch
)

if(NOT DEFINED TARGET_ARCH)
    message(FATAL_ERROR "TARGET_ARCH not set")
endif()

if(TARGET_ARCH STREQUAL "microbit")
    target_sources(temperature PRIVATE
        src/arch/microbit/temperature_microbit.cpp
    )
    target_link_libraries(temperature PRIVATE nrfx)
else()
    message(FATAL_ERROR "Unknown TARGET_ARCH: ${TARGET_ARCH}")
endif()
