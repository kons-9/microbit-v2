cmake_minimum_required(VERSION 3.16)

add_library(utkernel_cpp INTERFACE)
target_include_directories(utkernel_cpp INTERFACE
    include
    include/c  # extern "C" wrappers (must precede kernel includes for #include_next)
)

if(NOT DEFINED TARGET_ARCH)
    message(FATAL_ERROR "TARGET_ARCH not set")
endif()

target_compile_definitions(utkernel_cpp INTERFACE UTKERNEL_ARCH_${TARGET_ARCH})

if(TARGET_ARCH STREQUAL "microbit")
    target_link_libraries(utkernel_cpp INTERFACE kernel)
elseif(TARGET_ARCH STREQUAL "linux")
    target_link_libraries(utkernel_cpp INTERFACE pthread)
else()
    message(FATAL_ERROR "Unknown TARGET_ARCH: ${TARGET_ARCH}")
endif()
