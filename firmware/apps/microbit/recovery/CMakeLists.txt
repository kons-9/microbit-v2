add_executable(app_recovery.elf main.cpp)

target_link_libraries(app_recovery.elf PRIVATE
    shell
    fs
    io
    log
    drivers
    sysconfig
    utkernel_cpp
)

# リンカスクリプト
set(LINKER_SCRIPT ${CMAKE_SOURCE_DIR}/linker/recovery.ld)
target_link_options(app_recovery.elf PRIVATE
    -L${CMAKE_SOURCE_DIR}/linker
    -T${LINKER_SCRIPT}
    -Wl,-Map=app_recovery.map
)
