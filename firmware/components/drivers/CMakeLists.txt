cmake_minimum_required(VERSION 3.16)

add_library(drivers STATIC)

target_sources(drivers PRIVATE
    src/ble.cpp
    src/uart.cpp
)

target_include_directories(drivers
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(drivers
    PUBLIC io
    PRIVATE utkernel_cpp
)

if(NOT DEFINED TARGET_ARCH)
    message(FATAL_ERROR "TARGET_ARCH not set")
endif()

if(TARGET_ARCH STREQUAL "microbit")
    target_sources(drivers PRIVATE
        src/arch/microbit/ble_microbit.cpp
        src/arch/microbit/accelerometer_microbit.cpp
        src/arch/microbit/button_microbit.cpp
        src/arch/microbit/led_microbit.cpp
        src/arch/microbit/magnetometer_microbit.cpp
        src/arch/microbit/mic_microbit.cpp
        src/arch/microbit/speaker_microbit.cpp
        src/arch/microbit/temperature_microbit.cpp
        src/arch/microbit/touch_microbit.cpp
        src/arch/microbit/uart_microbit.cpp
    )
    target_link_libraries(drivers PRIVATE nrfx)
elseif(TARGET_ARCH STREQUAL "linux")
    target_sources(drivers PRIVATE
        src/arch/linux/uart_linux.cpp
    )
else()
    message(FATAL_ERROR "Unknown TARGET_ARCH: ${TARGET_ARCH}")
endif()
