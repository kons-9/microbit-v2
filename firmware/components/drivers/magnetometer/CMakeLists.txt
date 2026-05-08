cmake_minimum_required(VERSION 3.16)

add_library(magnetometer STATIC)

target_sources(magnetometer PRIVATE
    src/magnetometer.cpp
)

target_include_directories(magnetometer
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src/arch
)

if(NOT DEFINED TARGET_ARCH)
    message(FATAL_ERROR "TARGET_ARCH not set")
endif()

if(TARGET_ARCH STREQUAL "microbit")
    target_sources(magnetometer PRIVATE
        src/arch/microbit/magnetometer_microbit.cpp
    )
    target_link_libraries(magnetometer PRIVATE nrfx)
else()
    message(FATAL_ERROR "Unknown TARGET_ARCH: ${TARGET_ARCH}")
endif()
