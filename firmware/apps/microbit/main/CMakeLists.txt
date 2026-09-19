add_subdirectory(crash ${CMAKE_BINARY_DIR}/main_crash)
add_subdirectory(task ${CMAKE_BINARY_DIR}/main_task)

add_executable(app_main.elf
    main.cpp
    usermain.cpp
)

target_sources(app_main.elf PRIVATE
    $<TARGET_OBJECTS:main_crash>
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
set_target_properties(app_main.elf PROPERTIES
    LINK_DEPENDS "${LINKER_SCRIPT};${CMAKE_SOURCE_DIR}/linker/memory_map.ld"
)
target_link_options(app_main.elf PRIVATE
    -L${CMAKE_SOURCE_DIR}/linker
    -T${LINKER_SCRIPT}
    -Wl,-Map=app_main.map
)
