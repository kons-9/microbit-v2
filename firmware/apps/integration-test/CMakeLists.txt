add_subdirectory(${CMAKE_SOURCE_DIR}/apps/main/beacon
                 ${CMAKE_BINARY_DIR}/beacon)

add_executable(app_main.elf
    main.cpp
)

target_link_libraries(app_main.elf
    PRIVATE
        template
        utkernel_cpp
        io
        crash
        beacon
        shell
        flash_fs
        drivers
)

# リンカスクリプト (SoftDeviceなしのapp領域)
set(LINKER_SCRIPT ${CMAKE_SOURCE_DIR}/linker/app.ld)
target_link_options(app_main.elf PRIVATE
    -L${CMAKE_SOURCE_DIR}/linker
    -T${LINKER_SCRIPT}
    -Wl,-Map=app_main.map
)
