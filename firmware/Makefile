
# build/{release,debug}/{main, updater}/.../app_main.elf
IS_DEBUG ?= 0
BUILD_TYPE := $(if $(filter 1,$(IS_DEBUG)),debug,release)
EXTRA_C_FLAGS += $(if $(filter 1,$(IS_DEBUG)),-DDEBUG -O0 -g)
EXTRA_CXX_FLAGS += $(if $(filter 1,$(IS_DEBUG)),-DDEBUG -O0 -g)

MAIN_BIN := main/apps/main/app_main.elf
UPDATER_BIN := updater/apps/updater/app_updater.elf

MAIN:=build/$(BUILD_TYPE)/$(MAIN_BIN)
UPDATER:=build/$(BUILD_TYPE)/$(UPDATER_BIN)

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

.PHONY: all build build-updater clean erase-flash flash flash-updater format $(MAIN) $(UPDATER)

all: flash flash-updater

format:
	find apps components -name '*.c' -o -name '*.h' -o -name '*.cpp' -o -name '*.hpp' | grep -v 'third_party/' | xargs clang-format-18 -i

build: $(MAIN)

build-updater: $(UPDATER)

clean:
	rm -rf build
	
venv:
	python -m venv venv
	. venv/bin/activate
	pip install -r requirements.txt

# only one time
erase-flash: build venv
	if [ ! -f "erase-flash-done" ]; then \
		. venv/bin/activate; \
		pyocd erase --mass; \
		touch erase-flash-done; \
	fi

flash: erase-flash build venv
	. venv/bin/activate
	pyocd flash $(MAIN)

flash-updater: erase-flash build-updater venv
	. venv/bin/activate
	pyocd flash $(UPDATER)

$(MAIN):
	cmake -B build/$(BUILD_TYPE)/main -DAPP_TARGET=main -DCMAKE_EXPORT_COMPILE_COMMANDS=ON $(CMAKE_EXTRA_ARGS)
	cmake --build build/$(BUILD_TYPE)/main

$(UPDATER):
	cmake -B build/$(BUILD_TYPE)/updater -DAPP_TARGET=updater $(CMAKE_EXTRA_ARGS)
	cmake --build build/$(BUILD_TYPE)/updater
