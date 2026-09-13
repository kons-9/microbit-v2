cmake_minimum_required(VERSION 3.16)

add_library(sysconfig STATIC)

target_include_directories(sysconfig
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
)

if(TARGET_ARCH STREQUAL "microbit")
    target_compile_definitions(sysconfig PUBLIC SYSCONFIG_ARCH_MICROBIT)
    target_sources(sysconfig PRIVATE
        src/arch/microbit/sysconfig_microbit.cpp
    )
    target_link_libraries(sysconfig PRIVATE nrfx)
elseif(TARGET_ARCH STREQUAL "linux")
    target_compile_definitions(sysconfig PUBLIC SYSCONFIG_ARCH_LINUX)
    target_sources(sysconfig PRIVATE
        src/arch/linux/sysconfig_linux.cpp
    )
else()
    message(FATAL_ERROR "Unknown TARGET_ARCH: ${TARGET_ARCH}")
endif()
