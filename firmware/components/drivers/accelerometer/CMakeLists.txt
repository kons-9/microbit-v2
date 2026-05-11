cmake_minimum_required(VERSION 3.16)

add_library(accelerometer STATIC)

target_sources(accelerometer PRIVATE
    src/accelerometer.cpp
)

target_include_directories(accelerometer
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src/arch
)

if(NOT DEFINED TARGET_ARCH)
    message(FATAL_ERROR "TARGET_ARCH not set")
endif()

if(TARGET_ARCH STREQUAL "microbit")
    target_sources(accelerometer PRIVATE
        src/arch/microbit/accelerometer_microbit.cpp
    )
    target_link_libraries(accelerometer PRIVATE nrfx log)
else()
    message(FATAL_ERROR "Unknown TARGET_ARCH: ${TARGET_ARCH}")
endif()
