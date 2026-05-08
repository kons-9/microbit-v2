FIRMWARE:=build/apps/main/app_main.elf
UPDATER:=build_updater/apps/updater/app_updater.elf

.PHONY: all build build-updater clean erase-flash flash flash-updater format $(FIRMWARE) $(UPDATER)

all: flash flash-updater

format:
	find apps components -name '*.c' -o -name '*.h' -o -name '*.cpp' -o -name '*.hpp' | grep -v 'third_party/' | xargs clang-format-18 -i

build: $(FIRMWARE)

build-updater: $(UPDATER)

clean:
	rm -rf build build_updater
	
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
	pyocd flash $(FIRMWARE)

flash-updater: erase-flash build-updater venv
	. venv/bin/activate
	pyocd flash $(UPDATER)

$(FIRMWARE):
	cmake -B build -DAPP_TARGET=main -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
	cmake --build build

$(UPDATER):
	cmake -B build_updater -DAPP_TARGET=updater
	cmake --build build_updater
