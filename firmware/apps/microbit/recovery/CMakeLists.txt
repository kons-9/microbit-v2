add_executable(app_recovery.elf main.cpp)

target_include_directories(app_recovery.elf PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../main/crash/include
)

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
set_target_properties(app_recovery.elf PROPERTIES
    LINK_DEPENDS "${LINKER_SCRIPT};${CMAKE_SOURCE_DIR}/linker/memory_map.ld"
)
target_link_options(app_recovery.elf PRIVATE
    -L${CMAKE_SOURCE_DIR}/linker
    -T${LINKER_SCRIPT}
    -Wl,-Map=app_recovery.map
)
