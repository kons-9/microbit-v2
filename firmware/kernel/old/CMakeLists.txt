# kernel/CMakeLists.txt.makefile-wrap
# 旧バージョン: 独自Makefile経由でカーネルをビルドし IMPORTED ライブラリとして取り込む方式
# build_make/Makefile.kernel + microbit.mk と組み合わせて使う
#
# 参考として残す。現在は cmake/ 配下のソースリストで直接CMakeビルドに移行済み。

cmake_minimum_required(VERSION 3.16)

set(MTKERNEL_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/mtkernel_3)
set(MAKE_DIR ${MTKERNEL_ROOT}/build_make)
set(KERNEL_LIB ${MAKE_DIR}/libtkernel.a)

# mtkernel_3 は独自Makefileでビルド（ソース改変不要）
# Makefile.kernel: usermain除外 & libtkernel.a のみ生成
add_custom_command(
    OUTPUT ${KERNEL_LIB}
    COMMAND make -f ${CMAKE_CURRENT_SOURCE_DIR}/Makefile.kernel
    WORKING_DIRECTORY ${MAKE_DIR}
    COMMENT "Building MTKernel via Make"
    VERBATIM
)
add_custom_target(kernel_build DEPENDS ${KERNEL_LIB})

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
    PROHIBIT_DEF_SIZE_T
)
target_compile_options(kernel INTERFACE
    -include stddef.h
)
add_dependencies(kernel kernel_build)
