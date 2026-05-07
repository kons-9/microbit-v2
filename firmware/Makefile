FIRMWARE:=build/apps/main/app_main.elf

.PHONY: all build clean erase-flash flash format $(FIRMWARE)

all: flash

format:
	find apps components -name '*.c' -o -name '*.h' -o -name '*.cpp' -o -name '*.hpp' | grep -v 'third_party/' | xargs clang-format-18 -i

build: $(FIRMWARE)

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
	pyocd flash $(FIRMWARE)

$(FIRMWARE):
	cmake -B build -DAPP_TARGET=main -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
	cmake --build build
