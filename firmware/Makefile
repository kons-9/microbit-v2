
# build/{release,debug}/{main, updater}/.../app_main.elf
IS_DEBUG ?= 0
BUILD_TYPE := $(if $(filter 1,$(IS_DEBUG)),debug,release)
EXTRA_C_FLAGS += $(if $(filter 1,$(IS_DEBUG)),-DDEBUG -O0 -g)
EXTRA_CXX_FLAGS += $(if $(filter 1,$(IS_DEBUG)),-DDEBUG -O0 -g)

# SoftDevice hex (download from Nordic: nRF5 SDK > s140)
SOFTDEVICE_HEX := third_party/softdevice/s140_nrf52_7.3.0_softdevice.hex
SOFTDEVICE_URL := https://nsscprodmedia.blob.core.windows.net/prod/software-and-other-downloads/softdevices/s140/s140_nrf52_7.3.0.zip

MAIN_BIN := main/apps/main/app_main.elf
UPDATER_BIN := updater/apps/updater/app_updater.elf
FACTORY_TEST_BIN := factory-test/apps/factory-test/app_main.elf
SAMPLE_BIN := sample/apps/sample/app_main.elf
INTEGRATION_TEST_BIN := integration-test/apps/integration-test/app_main.elf
BLE_TEST_BIN := ble-test/apps/ble-test/app_main.elf

MAIN:=build/$(BUILD_TYPE)/$(MAIN_BIN)
UPDATER:=build/$(BUILD_TYPE)/$(UPDATER_BIN)
FACTORY_TEST:=build/$(BUILD_TYPE)/$(FACTORY_TEST_BIN)
SAMPLE:=build/$(BUILD_TYPE)/$(SAMPLE_BIN)
INTEGRATION_TEST:=build/$(BUILD_TYPE)/$(INTEGRATION_TEST_BIN)
BLE_TEST:=build/$(BUILD_TYPE)/$(BLE_TEST_BIN)

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

.PHONY: all build build-updater build-factory-test build-sample build-integration-test build-ble-test clean erase-flash flash flash-updater flash-sample flash-integration-test flash-ble-test format $(MAIN) $(UPDATER) $(FACTORY_TEST) $(SAMPLE) $(INTEGRATION_TEST) $(BLE_TEST)

all: flash flash-updater

format:
	find apps components -name '*.c' -o -name '*.h' -o -name '*.cpp' -o -name '*.hpp' | grep -v 'third_party/' | xargs clang-format-18 -i

build: $(MAIN)

build-updater: $(UPDATER)

build-factory-test: $(FACTORY_TEST)

build-sample: $(SAMPLE)

build-integration-test: $(INTEGRATION_TEST)

build-ble-test: $(BLE_TEST)

clean:
	rm -rf build

$(SOFTDEVICE_HEX):
	@echo "Downloading SoftDevice S140 v7.3.0..."
	@mkdir -p $(dir $@)
	curl -sL $(SOFTDEVICE_URL) -o /tmp/s140.zip
	unzip -o -j /tmp/s140.zip "s140_nrf52_7.3.0_softdevice.hex" -d $(dir $@)
	rm -f /tmp/s140.zip
	@echo "SoftDevice ready: $@"
	
venv:
	python -m venv venv
	. venv/bin/activate
	pip install -r requirements.txt

flash: build venv $(SOFTDEVICE_HEX)
	. venv/bin/activate
	pyocd flash -e chip $(SOFTDEVICE_HEX) $(MAIN)

flash-updater: build-updater venv $(SOFTDEVICE_HEX)
	. venv/bin/activate
	pyocd flash -e sector $(UPDATER)

flash-sample: build-sample venv
	. venv/bin/activate
	pyocd flash -e chip $(SAMPLE)

flash-factory-test: build-factory-test venv
	. venv/bin/activate
	pyocd flash -e chip $(FACTORY_TEST)

flash-integration-test: build-integration-test venv $(SOFTDEVICE_HEX)
	. venv/bin/activate
	pyocd flash -e chip $(SOFTDEVICE_HEX) $(INTEGRATION_TEST)

flash-ble-test: build-ble-test venv $(SOFTDEVICE_HEX)
	. venv/bin/activate
	pyocd flash -e chip $(SOFTDEVICE_HEX) $(BLE_TEST)

$(MAIN):
	cmake -B build/$(BUILD_TYPE)/main -DAPP_TARGET=main -DCMAKE_EXPORT_COMPILE_COMMANDS=ON $(CMAKE_EXTRA_ARGS)
	cmake --build build/$(BUILD_TYPE)/main

$(UPDATER):
	cmake -B build/$(BUILD_TYPE)/updater -DAPP_TARGET=updater $(CMAKE_EXTRA_ARGS)
	cmake --build build/$(BUILD_TYPE)/updater

$(FACTORY_TEST):
	cmake -B build/$(BUILD_TYPE)/factory-test -DAPP_TARGET=factory-test $(CMAKE_EXTRA_ARGS)
	cmake --build build/$(BUILD_TYPE)/factory-test

$(SAMPLE):
	cmake -B build/$(BUILD_TYPE)/sample -DAPP_TARGET=sample $(CMAKE_EXTRA_ARGS)
	cmake --build build/$(BUILD_TYPE)/sample

$(INTEGRATION_TEST):
	cmake -B build/$(BUILD_TYPE)/integration-test -DAPP_TARGET=integration-test $(CMAKE_EXTRA_ARGS)
	cmake --build build/$(BUILD_TYPE)/integration-test

$(BLE_TEST):
	cmake -B build/$(BUILD_TYPE)/ble-test -DAPP_TARGET=ble-test $(CMAKE_EXTRA_ARGS)
	cmake --build build/$(BUILD_TYPE)/ble-test

# ======== Unit Tests ========
TEST_BUILD_DIR := test/build

.PHONY: test test-build test-%

# 全テストをビルド＆実行
test: test-build
	cd $(TEST_BUILD_DIR) && ctest --output-on-failure

# テストのビルドだけ
test-build:
	cmake -B $(TEST_BUILD_DIR) test -DCMAKE_BUILD_TYPE=Debug
	cmake --build $(TEST_BUILD_DIR) -j$$(nproc)

# コンポーネント単位で実行: make test-flash_fs, make test-shell, etc.
test-%: test-build
	cd $(TEST_BUILD_DIR) && ctest --output-on-failure -R "\\[$*\\]"
