add_executable(app_main.elf
    main.cpp
)

target_link_libraries(app_main.elf
    PRIVATE
        template
        utkernel_cpp
        crash
        io
        drivers
)

# メインアプリ領域 (SoftDeviceなし、Updater/Flash FS領域は予約)
set(LINKER_SCRIPT ${CMAKE_SOURCE_DIR}/linker/app.ld)
target_link_options(app_main.elf PRIVATE
    -L${CMAKE_SOURCE_DIR}/linker
    -T${LINKER_SCRIPT}
    -Wl,-Map=app_main.map
)
