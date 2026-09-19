cmake_minimum_required(VERSION 3.16)
project(firmware_tests LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# VS Code などの IDE 向けに compile_commands.json を生成する。
# -DCMAKE_EXPORT_COMPILE_COMMANDS=OFF で無効化できる。
option(CMAKE_EXPORT_COMPILE_COMMANDS
       "Generate compile_commands.json for IDEs"
       ON)

# テスト対象は全て Linux ビルド
set(TARGET_ARCH "linux")

# ===== Catch2 (1回だけ取得・ビルドし全テストで共有) =====
include(FetchContent)
FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.5.2
)
FetchContent_MakeAvailable(Catch2)
list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)

enable_testing()
include(Catch)

# ===== コンポーネントライブラリ =====

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../components/utkernel-cpp
                 ${CMAKE_BINARY_DIR}/lib/utkernel-cpp)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../components/template
                 ${CMAKE_BINARY_DIR}/lib/template)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../components/io
                 ${CMAKE_BINARY_DIR}/lib/io)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../components/log
                 ${CMAKE_BINARY_DIR}/lib/log)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../components/sysconfig
                 ${CMAKE_BINARY_DIR}/lib/sysconfig)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../components/fs
                 ${CMAKE_BINARY_DIR}/lib/fs)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../components/shell
                 ${CMAKE_BINARY_DIR}/lib/shell)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../components/drivers
                 ${CMAKE_BINARY_DIR}/lib/drivers)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../components/font
                 ${CMAKE_BINARY_DIR}/lib/font)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../components/library
                 ${CMAKE_BINARY_DIR}/lib/library)
# ===== テスト実行ファイル =====

# --- utkernel-cpp ---
add_executable(utkernel_test
    ${CMAKE_CURRENT_SOURCE_DIR}/../components/utkernel-cpp/test/linux/test_mutex.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../components/utkernel-cpp/test/linux/test_semaphore.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../components/utkernel-cpp/test/linux/test_task.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../components/utkernel-cpp/test/linux/test_eventflag.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../components/utkernel-cpp/test/linux/test_msgbuf.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../components/utkernel-cpp/test/linux/test_timer.cpp
)
target_link_libraries(utkernel_test PRIVATE utkernel_cpp Catch2::Catch2WithMain)
catch_discover_tests(utkernel_test TEST_PREFIX "[utkernel] ")

# --- template ---
add_executable(template_test
    ${CMAKE_CURRENT_SOURCE_DIR}/../components/template/test/linux/template_test.cpp
)
target_link_libraries(template_test PRIVATE template Catch2::Catch2WithMain)
catch_discover_tests(template_test TEST_PREFIX "[template] ")

# --- fs ---
add_executable(fs_test
    ${CMAKE_CURRENT_SOURCE_DIR}/../components/fs/test/linux/test_fs.cpp
)
target_link_libraries(fs_test PRIVATE fs Catch2::Catch2WithMain)
catch_discover_tests(fs_test TEST_PREFIX "[fs] ")

# --- shell ---
add_executable(shell_test
    ${CMAKE_CURRENT_SOURCE_DIR}/../components/shell/test/linux/test_shell.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../components/shell/test/linux/mock_uart.cpp
)
target_link_libraries(shell_test PRIVATE shell fs Catch2::Catch2WithMain)
catch_discover_tests(shell_test TEST_PREFIX "[shell] ")

# --- log_format (log_vformat テスト) ---
add_executable(log_format_test
    ${CMAKE_CURRENT_SOURCE_DIR}/../components/io/test/linux/test_log_format.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../components/shell/test/linux/mock_uart.cpp
)
target_link_libraries(log_format_test PRIVATE log Catch2::Catch2WithMain)
catch_discover_tests(log_format_test TEST_PREFIX "[log_format] ")

# --- font5x5 ---
add_executable(font5x5_test
    ${CMAKE_CURRENT_SOURCE_DIR}/../components/font/test/linux/test_font5x5.cpp
)
target_link_libraries(font5x5_test PRIVATE font Catch2::Catch2WithMain)
catch_discover_tests(font5x5_test TEST_PREFIX "[font5x5] ")

# --- library ---
add_executable(library_test
    ${CMAKE_CURRENT_SOURCE_DIR}/../components/library/test/linux/test_library.cpp
)
target_link_libraries(library_test PRIVATE library Catch2::Catch2WithMain)
catch_discover_tests(library_test TEST_PREFIX "[library] ")
