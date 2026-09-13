
# build/{release,debug}/{main, updater}/.../app_main.elf
IS_DEBUG ?= 0
BUILD_TYPE := $(if $(filter 1,$(IS_DEBUG)),debug,release)
EXTRA_C_FLAGS += $(if $(filter 1,$(IS_DEBUG)),-DDEBUG -O0 -g)
EXTRA_CXX_FLAGS += $(if $(filter 1,$(IS_DEBUG)),-DDEBUG -O0 -g)

MAIN_BIN := main/apps/main/app_main.elf
UPDATER_BIN := updater/apps/updater/app_updater.elf
FACTORY_TEST_BIN := factory-test/apps/factory-test/app_main.elf
SAMPLE_BIN := sample/apps/sample/app_main.elf
INTEGRATION_TEST_BIN := integration-test/apps/integration-test/app_main.elf
ADVERTISE_TEST_BIN := advertise-test/apps/advertise-test/app_main.elf

MAIN:=build/$(BUILD_TYPE)/$(MAIN_BIN)
UPDATER:=build/$(BUILD_TYPE)/$(UPDATER_BIN)
FACTORY_TEST:=build/$(BUILD_TYPE)/$(FACTORY_TEST_BIN)
SAMPLE:=build/$(BUILD_TYPE)/$(SAMPLE_BIN)
INTEGRATION_TEST:=build/$(BUILD_TYPE)/$(INTEGRATION_TEST_BIN)
ADVERTISE_TEST:=build/$(BUILD_TYPE)/$(ADVERTISE_TEST_BIN)

# 外部から追加コンパイルフラグを渡す (e.g. make EXTRA_C_FLAGS="-DDEBUG_CRASH -O0")
EXTRA_C_FLAGS ?=
EXTRA_CXX_FLAGS ?=

CMAKE_EXTRA_ARGS :=
ifneq ($(EXTRA_C_FLAGS),)
  CMAKE_EXTRA_ARGS += -DEXTRA_C_FLAGS="$(EXTRA_C_FLAGS)"
endif
ifneq ($(EXTRA_CXX_FLAGS),)
  CMAKE_EXTRA_ARGS += -DEXTRA_CXX_FLAGS="$(EXTRA_CXX_FLAGS)"
endif

.PHONY: all build build-updater build-factory-test build-sample build-integration-test build-advertise-test clean erase-flash flash flash-updater flash-sample flash-integration-test flash-advertise-test flash-advertise-test-recover format $(MAIN) $(UPDATER) $(FACTORY_TEST) $(SAMPLE) $(INTEGRATION_TEST) $(ADVERTISE_TEST)

all: flash flash-updater

format:
	find apps components -name '*.c' -o -name '*.h' -o -name '*.cpp' -o -name '*.hpp' | grep -v 'third_party/' | xargs clang-format-18 -i

build: $(MAIN)

build-updater: $(UPDATER)

build-factory-test: $(FACTORY_TEST)

build-sample: $(SAMPLE)

build-integration-test: $(INTEGRATION_TEST)

build-advertise-test: $(ADVERTISE_TEST)

clean:
	rm -rf build

venv:
	python -m venv venv
	. venv/bin/activate
	pip install -r requirements.txt

flash: build venv
	. venv/bin/activate
	pyocd flash -e chip $(MAIN)

flash-updater: build-updater venv
	. venv/bin/activate
	pyocd flash -e sector $(UPDATER)

flash-sample: build-sample venv
	. venv/bin/activate
	pyocd flash -e chip $(SAMPLE)

flash-factory-test: build-factory-test venv
	. venv/bin/activate
	pyocd flash -e chip $(FACTORY_TEST)

flash-integration-test: build-integration-test venv
	. venv/bin/activate
	pyocd flash -e chip $(INTEGRATION_TEST)

# RADIO直叩きのAdvertising確認用。SoftDeviceは書き込まない。
flash-advertise-test: build-advertise-test venv
	. venv/bin/activate
	pyocd flash -e chip $(ADVERTISE_TEST)

# SoftDevice搭載済みの個体を初めてスタンドアロンへ切り替える場合に1回だけ実行する。
# 全フラッシュを消去するため、既存のファームウェア・ログ・設定も消える。
flash-advertise-test-recover: build-advertise-test venv
	venv/bin/pyocd erase --mass
	venv/bin/pyocd flash --erase=auto $(ADVERTISE_TEST)

$(MAIN):
	cmake -B build/$(BUILD_TYPE)/main -DAPP_TARGET=main -DCMAKE_EXPORT_COMPILE_COMMANDS=ON $(CMAKE_EXTRA_ARGS)
	cmake --build build/$(BUILD_TYPE)/main

$(UPDATER):
	cmake -B build/$(BUILD_TYPE)/updater -DAPP_TARGET=updater -DCMAKE_EXPORT_COMPILE_COMMANDS=ON $(CMAKE_EXTRA_ARGS)
	cmake --build build/$(BUILD_TYPE)/updater

$(FACTORY_TEST):
	cmake -B build/$(BUILD_TYPE)/factory-test -DAPP_TARGET=factory-test -DCMAKE_EXPORT_COMPILE_COMMANDS=ON $(CMAKE_EXTRA_ARGS)
	cmake --build build/$(BUILD_TYPE)/factory-test

$(SAMPLE):
	cmake -B build/$(BUILD_TYPE)/sample -DAPP_TARGET=sample -DCMAKE_EXPORT_COMPILE_COMMANDS=ON $(CMAKE_EXTRA_ARGS)
	cmake --build build/$(BUILD_TYPE)/sample

$(INTEGRATION_TEST):
	cmake -B build/$(BUILD_TYPE)/integration-test -DAPP_TARGET=integration-test -DCMAKE_EXPORT_COMPILE_COMMANDS=ON $(CMAKE_EXTRA_ARGS)
	cmake --build build/$(BUILD_TYPE)/integration-test

$(ADVERTISE_TEST):
	cmake -B build/$(BUILD_TYPE)/advertise-test -DAPP_TARGET=advertise-test -DCMAKE_EXPORT_COMPILE_COMMANDS=ON $(CMAKE_EXTRA_ARGS)
	cmake --build build/$(BUILD_TYPE)/advertise-test

# ======== Unit Tests ========
TEST_BUILD_DIR := test/build

.PHONY: test test-build test-%

# 全テストをビルド＆実行
test: test-build
	cd $(TEST_BUILD_DIR) && ctest --output-on-failure

# テストのビルドだけ
test-build:
	cmake -B $(TEST_BUILD_DIR) test -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
	cmake --build $(TEST_BUILD_DIR) -j$$(nproc)

# コンポーネント単位で実行: make test-flash_fs, make test-shell, etc.
test-%: test-build
	cd $(TEST_BUILD_DIR) && ctest --output-on-failure -R "\\[$*\\]"
