cmake_minimum_required(VERSION 3.16)

add_library(uart STATIC)

target_sources(uart PRIVATE
    src/uart.cpp
)

target_include_directories(uart
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src/arch
)

if(NOT DEFINED TARGET_ARCH)
    message(FATAL_ERROR "TARGET_ARCH not set")
endif()

if(TARGET_ARCH STREQUAL "linux")
    target_sources(uart PRIVATE
        src/arch/linux/uart_linux.cpp
    )
elseif(TARGET_ARCH STREQUAL "microbit")
    target_sources(uart PRIVATE
        src/arch/microbit/uart_microbit.cpp
    )
    target_link_libraries(uart PRIVATE nrfx)
else()
    message(FATAL_ERROR "Unknown TARGET_ARCH: ${TARGET_ARCH}")
endif()

set_target_properties(uart PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/uart
)
