cmake_minimum_required(VERSION 3.16)

add_library(flash_fs STATIC)

target_sources(flash_fs PRIVATE
    src/flash_fs.cpp
)

target_include_directories(flash_fs
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

if(NOT DEFINED TARGET_ARCH)
    message(FATAL_ERROR "TARGET_ARCH not set")
endif()

if(TARGET_ARCH STREQUAL "microbit")
    target_sources(flash_fs PRIVATE
        src/arch/microbit/flash_fs_microbit.cpp
    )
    target_link_libraries(flash_fs PRIVATE nrfx io)
elseif(TARGET_ARCH STREQUAL "linux")
    target_sources(flash_fs PRIVATE
        src/arch/linux/flash_fs_linux.cpp
    )
    target_link_libraries(flash_fs PRIVATE io)
    target_compile_definitions(flash_fs PRIVATE FLASH_FS_ARCH_LINUX)
else()
    message(FATAL_ERROR "Unknown TARGET_ARCH: ${TARGET_ARCH}")
endif()

set_target_properties(flash_fs PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/flash_fs
)
