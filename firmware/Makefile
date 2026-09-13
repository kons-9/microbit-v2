
# build/{release,debug}/{main, recovery}/.../app_*.elf
IS_DEBUG ?= 0
BUILD_TYPE := $(if $(filter 1,$(IS_DEBUG)),debug,release)
EXTRA_C_FLAGS += $(if $(filter 1,$(IS_DEBUG)),-DDEBUG -O0 -g)
EXTRA_CXX_FLAGS += $(if $(filter 1,$(IS_DEBUG)),-DDEBUG -O0 -g)

MAIN_BIN := main/apps/microbit/main/app_main.elf
RECOVERY_BIN := recovery/apps/microbit/recovery/app_recovery.elf
SAMPLE_BIN := sample/apps/microbit/sample/app_main.elf
INTEGRATION_TEST_BIN := integration-test/apps/microbit/integration-test/app_main.elf

MAIN:=build/$(BUILD_TYPE)/$(MAIN_BIN)
RECOVERY:=build/$(BUILD_TYPE)/$(RECOVERY_BIN)
SAMPLE:=build/$(BUILD_TYPE)/$(SAMPLE_BIN)
INTEGRATION_TEST:=build/$(BUILD_TYPE)/$(INTEGRATION_TEST_BIN)

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

.PHONY: all build build-recovery build-sample build-integration-test clean erase-flash flash flash-recovery flash-sample flash-integration-test format $(MAIN) $(RECOVERY) $(SAMPLE) $(INTEGRATION_TEST)

all: flash flash-recovery

format:
	find apps components -name '*.c' -o -name '*.h' -o -name '*.cpp' -o -name '*.hpp' | grep -v 'third_party/' | xargs clang-format-18 -i
	
build-all: build build-recovery build-sample build-integration-test

build: $(MAIN)

build-recovery: $(RECOVERY)

build-sample: $(SAMPLE)

build-integration-test: $(INTEGRATION_TEST)

clean:
	rm -rf build

venv:
	python -m venv venv
	. venv/bin/activate
	pip install -r requirements.txt

flash: build venv
	. venv/bin/activate
	pyocd flash -e chip $(MAIN)

flash-recovery: build-recovery venv
	. venv/bin/activate
	pyocd flash -e sector $(RECOVERY)

flash-sample: build-sample venv
	. venv/bin/activate
	pyocd flash -e chip $(SAMPLE)

flash-integration-test: build-integration-test venv
	. venv/bin/activate
	pyocd flash -e chip $(INTEGRATION_TEST)

$(MAIN):
	cmake -B build/$(BUILD_TYPE)/main -DAPP_TARGET=main $(CMAKE_EXTRA_ARGS)
	cmake --build build/$(BUILD_TYPE)/main

$(RECOVERY):
	cmake -B build/$(BUILD_TYPE)/recovery -DAPP_TARGET=recovery $(CMAKE_EXTRA_ARGS)
	cmake --build build/$(BUILD_TYPE)/recovery

$(SAMPLE):
	cmake -B build/$(BUILD_TYPE)/sample -DAPP_TARGET=sample $(CMAKE_EXTRA_ARGS)
	cmake --build build/$(BUILD_TYPE)/sample

$(INTEGRATION_TEST):
	cmake -B build/$(BUILD_TYPE)/integration-test -DAPP_TARGET=integration-test $(CMAKE_EXTRA_ARGS)
	cmake --build build/$(BUILD_TYPE)/integration-test

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

# コンポーネント単位で実行: make test-fs, make test-shell, etc.
test-%: test-build
	cd $(TEST_BUILD_DIR) && ctest --output-on-failure -R "\\[$*\\]"
