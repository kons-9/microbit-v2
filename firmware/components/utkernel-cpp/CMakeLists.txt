cmake_minimum_required(VERSION 3.16)

add_library(utkernel_cpp STATIC)

target_sources(utkernel_cpp PRIVATE
    src/template.cpp
)

target_include_directories(utkernel_cpp
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include/c
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src/arch
)

if(NOT DEFINED TARGET_ARCH)
    message(FATAL_ERROR "TARGET_ARCH not set")
endif()

# 出力を同一ディレクトリに統一
set_target_properties(utkernel_cpp PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/utkernel_cpp
)

# ===== test (linux only) =====
if(TARGET_ARCH STREQUAL "linux")
    enable_testing()

    add_executable(utkernel_cpp_test
        test/linux/template_test.cpp
    )

    target_link_libraries(utkernel_cpp_test PRIVATE utkernel_cpp)

    set_target_properties(utkernel_cpp_test PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/utkernel_cpp
    )

    add_test(NAME utkernel_cpp_test COMMAND utkernel_cpp_test)
endif()