#include <catch2/catch_test_macros.hpp>
#include <osal/msgbuf>
#include <osal/task>
#include <cstring>

TEST_CASE("msgbuf: basic send/receive", "[msgbuf]") {
    osal::message_buffer mb(256, 64);
    const char msg[] = "hello";
    REQUIRE(mb.send(msg, sizeof(msg)) == true);

    char buf[64] = {};
    size_t n = mb.receive(buf, sizeof(buf));
    REQUIRE(n == sizeof(msg));
    REQUIRE(std::strcmp(buf, "hello") == 0);
}

TEST_CASE("msgbuf: multiple messages", "[msgbuf]") {
    osal::message_buffer mb(512, 64);

    uint32_t val1 = 0xDEADBEEF;
    uint32_t val2 = 0xCAFEBABE;
    REQUIRE(mb.send(&val1, sizeof(val1)) == true);
    REQUIRE(mb.send(&val2, sizeof(val2)) == true);

    uint32_t out = 0;
    REQUIRE(mb.receive(&out, sizeof(out)) == sizeof(uint32_t));
    REQUIRE(out == 0xDEADBEEF);
    REQUIRE(mb.receive(&out, sizeof(out)) == sizeof(uint32_t));
    REQUIRE(out == 0xCAFEBABE);
}

TEST_CASE("msgbuf: receive timeout", "[msgbuf]") {
    osal::message_buffer mb(256, 64);
    char buf[64];
    size_t n = mb.receive(buf, sizeof(buf), 50);
    REQUIRE(n == 0);
}

static osal::message_buffer *g_mb_ptr = nullptr;

static void sender_entry(void *) {
    osal::task::sleep_for(20);
    uint32_t val = 12345;
    g_mb_ptr->send(&val, sizeof(val));
}

TEST_CASE("msgbuf: cross-thread send/receive", "[msgbuf]") {
    osal::message_buffer mb(256, 64);
    g_mb_ptr = &mb;

    osal::task::config cfg;
    cfg.stack_size = 4096;
    osal::task t(sender_entry, cfg);

    uint32_t out = 0;
    size_t n = mb.receive(&out, sizeof(out), 1000);
    REQUIRE(n == sizeof(uint32_t));
    REQUIRE(out == 12345);
    t.join();
}

TEST_CASE("msgbuf: zero size rejected", "[msgbuf]") {
    osal::message_buffer mb(256, 64);
    REQUIRE(mb.send(nullptr, 0) == false);
}
