cmake_minimum_required(VERSION 3.16)

# Header-only library
add_library(osal INTERFACE)

target_include_directories(osal
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

if(NOT DEFINED TARGET_ARCH)
    message(FATAL_ERROR "TARGET_ARCH not set")
endif()

# ARCH define for header dispatch
target_compile_definitions(osal INTERFACE OSAL_ARCH_${TARGET_ARCH})

if(TARGET_ARCH STREQUAL "linux")
    target_link_libraries(osal INTERFACE pthread)
elseif(TARGET_ARCH STREQUAL "microbit")
    target_link_libraries(osal INTERFACE kernel)
else()
    message(FATAL_ERROR "Unknown TARGET_ARCH: ${TARGET_ARCH}")
endif()

# テストは firmware/test/CMakeLists.txt で統合管理
