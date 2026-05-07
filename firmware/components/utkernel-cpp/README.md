# component template
## Linuxテスト
cmake -B build/test -DTARGET_ARCH=linux
cmake --build build/test
ctest --test-dir build/test

## firmware（microbit）
cmake -B build/fw -DTARGET_ARCH=microbit
cmake --build build/fw