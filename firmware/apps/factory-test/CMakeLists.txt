add_executable(app_main.elf
    main.cpp
)

target_link_libraries(app_main.elf
    PRIVATE
        template
)

# リンカスクリプト
set(LINKER_SCRIPT
    ${CMAKE_SOURCE_DIR}/kernel/mtkernel_3/etc/linker/${TARGET_ARCH}/tkernel_map.ld
)
target_link_options(app_main.elf PRIVATE
    -T${LINKER_SCRIPT}
    -Wl,-Map=app_main.map
)
