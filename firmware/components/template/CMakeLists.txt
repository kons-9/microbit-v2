cmake_minimum_required(VERSION 3.16)

add_library(template STATIC)

target_sources(template PRIVATE
    src/template.cpp
)

target_include_directories(template
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src/arch
)

if(NOT DEFINED TARGET_ARCH)
    message(FATAL_ERROR "TARGET_ARCH not set")
endif()

if(TARGET_ARCH STREQUAL "linux")
    target_sources(template PRIVATE
        src/arch/linux/template_linux.cpp
    )
elseif(TARGET_ARCH STREQUAL "microbit")
    target_sources(template PRIVATE
        src/arch/microbit/template_microbit.cpp
    )
else()
    message(FATAL_ERROR "Unknown TARGET_ARCH")
endif()

# 出力を同一ディレクトリに統一
set_target_properties(template PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/template
)

# ===== test (linux only) =====
if(TARGET_ARCH STREQUAL "linux")
    enable_testing()

    add_executable(template_test
        test/linux/template_test.cpp
    )

    target_link_libraries(template_test PRIVATE template)

    set_target_properties(template_test PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/template
    )

    add_test(NAME template_test COMMAND template_test)
endif()