cmake_minimum_required(VERSION 3.16)

set(MTKERNEL_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/mtkernel_3)
set(MAKE_DIR ${MTKERNEL_ROOT}/build_make)

# ---- make実行 ----
add_custom_target(kernel_build
    COMMAND make -f ${CMAKE_CURRENT_SOURCE_DIR}/Makefile.kernel
    WORKING_DIRECTORY ${MAKE_DIR}
    COMMENT "Building MTKernel via Make"
)

# ---- 生成物（ここは実物に合わせる）----
set(KERNEL_LIB ${MAKE_DIR}/libtkernel.a)

add_library(kernel STATIC IMPORTED GLOBAL)

set_target_properties(kernel PROPERTIES
    IMPORTED_LOCATION ${KERNEL_LIB}
)

target_include_directories(kernel INTERFACE
    ${MTKERNEL_ROOT}/include
    ${MTKERNEL_ROOT}/config
)

target_compile_definitions(kernel INTERFACE
    TARGET_DIR=microbit
)

add_dependencies(kernel kernel_build)