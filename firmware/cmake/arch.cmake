if(NOT DEFINED TARGET_ARCH)
    message(FATAL_ERROR "TARGET_ARCH required")
endif()

if(TARGET_ARCH STREQUAL "linux")
    add_compile_definitions(ARCH_linux)
    message(FATAL_ERROR "linux is not supported")
elseif(TARGET_ARCH STREQUAL "microbit")
    add_compile_definitions(ARCH_microbit BOARD_microbit)
else()
    message(FATAL_ERROR "Unknown TARGET_ARCH=${TARGET_ARCH}")
endif()

message(STATUS "ARCH=${TARGET_ARCH}")
