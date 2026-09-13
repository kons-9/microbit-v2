add_executable(app_main.elf
    main.cpp
    src/test_context.cpp
    src/test_uart.cpp
    src/test_sysconfig.cpp
    src/test_fs.cpp
    src/test_ble.cpp
    src/test_shell.cpp
)

target_include_directories(app_main.elf PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(app_main.elf
    PRIVATE
        template
        utkernel_cpp
        io
        shell
        fs
        drivers
)

# リンカスクリプト (SoftDeviceなしのapp領域)
set(LINKER_SCRIPT ${CMAKE_SOURCE_DIR}/linker/app.ld)
target_link_options(app_main.elf PRIVATE
    -L${CMAKE_SOURCE_DIR}/linker
    -T${LINKER_SCRIPT}
    -Wl,-Map=app_main.map
)
