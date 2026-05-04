static int _main();

extern "C" int usermain(void) { return _main(); };

static int _main() {
    return 0;
}

