cmake_minimum_required(VERSION 3.16)

if(NOT DEFINED TARGET_ARCH)
    message(FATAL_ERROR "TARGET_ARCH not set")
endif()

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