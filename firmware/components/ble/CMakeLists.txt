add_library(ble STATIC
    src/ble.c
)

target_include_directories(ble
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)
