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

# ===== test (linux only) =====
if(TARGET_ARCH STREQUAL "linux")
    include(FetchContent)
    FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG        v3.5.2
    )
    FetchContent_MakeAvailable(Catch2)

    enable_testing()

    add_executable(osal_test
        test/linux/test_mutex.cpp
        test/linux/test_semaphore.cpp
        test/linux/test_task.cpp
        test/linux/test_eventflag.cpp
        test/linux/test_msgbuf.cpp
        test/linux/test_timer.cpp
    )

    target_link_libraries(osal_test PRIVATE osal Catch2::Catch2WithMain)

    set_target_properties(osal_test PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/osal
    )

    include(Catch)
    catch_discover_tests(osal_test)
endif()
