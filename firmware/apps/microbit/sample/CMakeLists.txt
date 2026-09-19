add_executable(app_main.elf
    main.cpp
)

target_link_libraries(app_main.elf
    PRIVATE
        template
        utkernel_cpp
)

# リンカスクリプト (standalone: SoftDevice無し、ROM 0x0始まり)
set(LINKER_SCRIPT ${CMAKE_SOURCE_DIR}/linker/standalone.ld)
set_target_properties(app_main.elf PROPERTIES
    LINK_DEPENDS "${LINKER_SCRIPT}"
)
target_link_options(app_main.elf PRIVATE
    -L${CMAKE_SOURCE_DIR}/linker
    -T${LINKER_SCRIPT}
    -Wl,-Map=app_main.map
)
