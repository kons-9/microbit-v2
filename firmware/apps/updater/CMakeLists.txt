add_executable(app_updater.elf main.cpp)

target_link_libraries(app_updater.elf PRIVATE
    shell
    fs
    io
    log
    drivers
    sysconfig
    utkernel_cpp
)

# リンカスクリプト
set(LINKER_SCRIPT ${CMAKE_SOURCE_DIR}/linker/updater.ld)
target_link_options(app_updater.elf PRIVATE
    -L${CMAKE_SOURCE_DIR}/linker
    -T${LINKER_SCRIPT}
    -Wl,-Map=app_updater.map
)
