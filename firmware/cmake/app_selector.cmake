if(NOT DEFINED APP_TARGET)
    message(FATAL_ERROR "APP_TARGET required")
endif()

if(APP_TARGET STREQUAL "main")
    set(APP_DIR apps/main)
elseif(APP_TARGET STREQUAL "factory-test")
    set(APP_DIR apps/factory-test)
else()
    message(FATAL_ERROR "unknown APP_TARGET")
endif()

add_subdirectory(${APP_DIR})