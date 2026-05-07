cmake_minimum_required(VERSION 3.16)

set(MTKERNEL_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/mtkernel_3)

# ソースリスト読み込み
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/sources_common.cmake)

if(TARGET_ARCH STREQUAL "microbit")
    include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/sources_microbit.cmake)
else()
    message(FATAL_ERROR "Unknown TARGET_ARCH for kernel: ${TARGET_ARCH}")
endif()

add_library(kernel STATIC
    ${KERNEL_COMMON_SOURCES}
    ${KERNEL_ARCH_SOURCES}
)

target_include_directories(kernel
    PUBLIC
        ${MTKERNEL_ROOT}/include
        ${MTKERNEL_ROOT}/config
    PRIVATE
        ${MTKERNEL_ROOT}/kernel/knlinc
)

target_compile_definitions(kernel
    PUBLIC
        TARGET_DIR=microbit
        PROHIBIT_DEF_SIZE_T
    PRIVATE
        ${KERNEL_ARCH_COMPILE_DEFINITIONS}
)

target_compile_options(kernel
    PUBLIC
        $<$<COMPILE_LANGUAGE:C,CXX>:-include stddef.h>
    PRIVATE
        $<$<COMPILE_LANGUAGE:C>:-std=gnu11>
        -ffunction-sections
        -fdata-sections
)