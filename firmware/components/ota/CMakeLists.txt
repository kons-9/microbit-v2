cmake_minimum_required(VERSION 3.16)

add_library(ota STATIC)

target_sources(ota PRIVATE
    src/ota.c
)

target_include_directories(ota
    PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(ota
    PUBLIC  sysconfig
    PRIVATE ble
)

if(TARGET_ARCH STREQUAL "microbit")
    target_sources(ota PRIVATE src/arch/microbit/ota_microbit.c)
    target_link_libraries(ota PRIVATE nrfx)
else()
    message(FATAL_ERROR "Unknown TARGET_ARCH: ${TARGET_ARCH}")
endif()
