extern "C" int app_main(void);
extern "C" void __libc_init_array(void);

extern "C" int usermain(void) {
    __libc_init_array();
    return app_main();
}
