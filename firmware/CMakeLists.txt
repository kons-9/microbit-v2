cmake_minimum_required(VERSION 3.16)

set(TARGET_ARCH "linux" CACHE STRING "Target architecture")
set_property(CACHE TARGET_ARCH PROPERTY STRINGS linux microbit)

set(APP_TARGET "main" CACHE STRING "Application target")
set_property(CACHE APP_TARGET PROPERTY STRINGS main factory-test)

if(NOT TARGET_ARCH STREQUAL "linux" AND NOT TARGET_ARCH STREQUAL "microbit")
    message(FATAL_ERROR "TARGET_ARCH must be linux or microbit")
endif()

if(TARGET_ARCH STREQUAL "microbit")
    include(cmake/toolchain.cmake)
endif()

project(ble_locator LANGUAGES C CXX)

message(STATUS "TARGET_ARCH=${TARGET_ARCH}")
message(STATUS "APP_TARGET=${APP_TARGET}")

include(cmake/options.cmake)
include(cmake/arch.cmake)
include(cmake/components.cmake)
include(cmake/app_selector.cmake)

#add_executable(app_main.elf)

set(LINKER_SCRIPT
    ${CMAKE_SOURCE_DIR}/kernel/mtkernel_3/etc/linker/${TARGET_ARCH}/tkernel_map.ld
)

target_link_options(app_main.elf PRIVATE
    -T${LINKER_SCRIPT}
    -Wl,-Map=app_main.map
)