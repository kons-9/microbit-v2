cmake_minimum_required(VERSION 3.14)
project(ble_locator LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Platform selection: SIM (default), MICROBIT
if(NOT DEFINED UAI_PLATFORM)
    set(UAI_PLATFORM "SIM")
endif()

# μAI-Bridge middleware
set(MIDDLEWARE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../uai-bridge")
set(MIDDLEWARE_INCLUDE "${MIDDLEWARE_DIR}/include")

# Application sources
set(APP_SOURCES
    main.cpp
    ble_scanner.cpp
    ble_service.cpp
    location.cpp
    estimation_mode.cpp
)

# Architecture-dependent sources
if(UAI_PLATFORM STREQUAL "SIM")
    list(APPEND APP_SOURCES arch/sim/arch_sim.cpp)
    add_definitions(-DUAI_PLATFORM_POSIX)
elseif(UAI_PLATFORM STREQUAL "MICROBIT")
    list(APPEND APP_SOURCES arch/microbit/arch_microbit.cpp)
    add_definitions(-DUAI_PLATFORM_MICROBIT)
endif()

# Middleware library
add_library(uai_bridge_lib STATIC
    ${MIDDLEWARE_DIR}/src/core/uai_protocol.cpp
    ${MIDDLEWARE_DIR}/src/transport/uai_tcp.cpp
)
target_include_directories(uai_bridge_lib PUBLIC ${MIDDLEWARE_INCLUDE})
if(UAI_PLATFORM STREQUAL "SIM")
    target_compile_definitions(uai_bridge_lib PRIVATE UAI_PLATFORM_POSIX)
endif()

# BLE Locator executable
add_executable(ble_locator ${APP_SOURCES})
target_include_directories(ble_locator PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${MIDDLEWARE_INCLUDE}
)
target_link_libraries(ble_locator PRIVATE uai_bridge_lib)

if(WIN32)
    target_link_libraries(ble_locator PRIVATE ws2_32)
    target_link_libraries(uai_bridge_lib PRIVATE ws2_32)
endif()
