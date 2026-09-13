add_subdirectory(crash ${CMAKE_BINARY_DIR}/main_crash)
add_subdirectory(task ${CMAKE_BINARY_DIR}/main_task)

add_executable(app_main.elf
    main.cpp
)

target_link_libraries(app_main.elf
    PRIVATE
        template
        utkernel_cpp
        main_crash
        io
        drivers
        fs
        main_task
)

# メインアプリ領域 (SoftDeviceなし、Recovery/Flash FS領域は予約)
set(LINKER_SCRIPT ${CMAKE_SOURCE_DIR}/linker/app.ld)
target_link_options(app_main.elf PRIVATE
    -L${CMAKE_SOURCE_DIR}/linker
    -T${LINKER_SCRIPT}
    -Wl,-Map=app_main.map
)
