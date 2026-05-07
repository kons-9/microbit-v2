cmake_minimum_required(VERSION 3.16)

if(NOT DEFINED TARGET_ARCH)
    message(FATAL_ERROR "TARGET_ARCH not set")
endif()

add_library(utkernel_cpp INTERFACE)
target_include_directories(utkernel_cpp INTERFACE include)
