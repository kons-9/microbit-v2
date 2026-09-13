cmake_minimum_required(VERSION 3.16)
project(firmware_tests LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

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
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../components/flash_fs
                 ${CMAKE_BINARY_DIR}/lib/flash_fs)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../components/shell
                 ${CMAKE_BINARY_DIR}/lib/shell)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../components/drivers
                 ${CMAKE_BINARY_DIR}/lib/drivers)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../components/font
                 ${CMAKE_BINARY_DIR}/lib/font)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../components/library
                 ${CMAKE_BINARY_DIR}/lib/library)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../apps/main/beacon
                 ${CMAKE_BINARY_DIR}/lib/beacon)

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

# --- flash_fs ---
add_executable(flash_fs_test
    ${CMAKE_CURRENT_SOURCE_DIR}/../components/flash_fs/test/linux/test_flash_fs.cpp
)
target_link_libraries(flash_fs_test PRIVATE flash_fs Catch2::Catch2WithMain)
catch_discover_tests(flash_fs_test TEST_PREFIX "[flash_fs] ")

# --- shell ---
add_executable(shell_test
    ${CMAKE_CURRENT_SOURCE_DIR}/../components/shell/test/linux/test_shell.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../components/shell/test/linux/mock_uart.cpp
)
target_link_libraries(shell_test PRIVATE shell flash_fs Catch2::Catch2WithMain)
catch_discover_tests(shell_test TEST_PREFIX "[shell] ")

# --- flash_log (header-only, flash_fs 依存) ---
add_executable(flash_log_test
    ${CMAKE_CURRENT_SOURCE_DIR}/../components/flash_fs/test/linux/test_flash_log.cpp
)
target_link_libraries(flash_log_test PRIVATE flash_fs Catch2::Catch2WithMain)
catch_discover_tests(flash_log_test TEST_PREFIX "[flash_log] ")

# --- log_format (log_vformat テスト) ---
add_executable(log_format_test
    ${CMAKE_CURRENT_SOURCE_DIR}/../components/io/test/linux/test_log_format.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../components/shell/test/linux/mock_uart.cpp
)
target_link_libraries(log_format_test PRIVATE io Catch2::Catch2WithMain)
catch_discover_tests(log_format_test TEST_PREFIX "[log_format] ")

# --- beacon ---
add_executable(beacon_test
    ${CMAKE_CURRENT_SOURCE_DIR}/../apps/main/beacon/test/linux/test_beacon.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../apps/main/beacon/test/linux/mock_ble.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../components/shell/test/linux/mock_uart.cpp
)
target_link_libraries(beacon_test PRIVATE beacon shell flash_fs Catch2::Catch2WithMain)
target_include_directories(beacon_test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../components/drivers/include
)
catch_discover_tests(beacon_test TEST_PREFIX "[beacon] ")

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
