add_executable(app_main.elf
    main.cpp
)

target_link_libraries(app_main.elf
    PRIVATE
        template
        utkernel_cpp
        io
        drivers
)

# RADIO直叩きの単体検証。SoftDeviceは使用しない。
set(LINKER_SCRIPT ${CMAKE_SOURCE_DIR}/linker/standalone.ld)
target_link_options(app_main.elf PRIVATE
    -L${CMAKE_SOURCE_DIR}/linker
    -T${LINKER_SCRIPT}
    -Wl,-Map=app_main.map
)
