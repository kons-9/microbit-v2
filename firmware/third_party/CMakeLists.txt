# nrfx library for micro:bit v2.2 (nRF52833)
# submodule 内にはファイルを追加しない。ここでラップする。

if(TARGET_ARCH STREQUAL "microbit")
    set(NRFX_DIR ${CMAKE_CURRENT_SOURCE_DIR}/nrfx)

    set(NRFX_SOURCES
        ${NRFX_DIR}/drivers/src/nrfx_gpiote.c
        ${NRFX_DIR}/drivers/src/nrfx_nvmc.c
        ${NRFX_DIR}/drivers/src/nrfx_pwm.c
        ${NRFX_DIR}/drivers/src/nrfx_saadc.c
        ${NRFX_DIR}/drivers/src/nrfx_timer.c
        ${NRFX_DIR}/drivers/src/nrfx_twim.c
        ${NRFX_DIR}/drivers/src/nrfx_temp.c
        ${NRFX_DIR}/lib/nrfx_atomic.c
        ${NRFX_DIR}/bsp/stable/mdk/system_nrf52833.c
    )

    add_library(nrfx STATIC ${NRFX_SOURCES})

    target_include_directories(nrfx SYSTEM PUBLIC
        ${NRFX_DIR}
        ${NRFX_DIR}/hal
        ${NRFX_DIR}/drivers
        ${NRFX_DIR}/drivers/include
        ${NRFX_DIR}/helpers
        ${NRFX_DIR}/lib
        ${NRFX_DIR}/bsp/stable
        ${NRFX_DIR}/bsp/stable/mdk
        ${CMAKE_CURRENT_SOURCE_DIR}/cmsis/Include
        ${CMAKE_CURRENT_SOURCE_DIR}/config
    )

    target_compile_definitions(nrfx PUBLIC
        NRF52833_XXAA
    )
endif()
