#include <catch2/catch_test_macros.hpp>
#include <utkernel/msgbuf>
#include <utkernel/task>
#include <cstring>

TEST_CASE("message_buffer send/receive", "[msgbuf]") {
    utkernel::message_buffer mb(256, 64);
    const char msg[] = "hello";
    REQUIRE(mb.send(msg, sizeof(msg)));

    char buf[64] = {};
    size_t len = mb.receive(buf, sizeof(buf));
    REQUIRE(len == sizeof(msg));
    REQUIRE(std::strcmp(buf, "hello") == 0);
}

TEST_CASE("message_buffer receive timeout", "[msgbuf]") {
    utkernel::message_buffer mb(256, 64);
    char buf[64] = {};
    size_t len = mb.receive(buf, sizeof(buf), 10);
    REQUIRE(len == 0);
}

TEST_CASE("message_buffer multiple messages", "[msgbuf]") {
    utkernel::message_buffer mb(512, 64);

    uint32_t val1 = 0xDEADBEEF;
    uint32_t val2 = 0xCAFEBABE;
    REQUIRE(mb.send(&val1, sizeof(val1)));
    REQUIRE(mb.send(&val2, sizeof(val2)));

    uint32_t out = 0;
    REQUIRE(mb.receive(&out, sizeof(out)) == sizeof(uint32_t));
    REQUIRE(out == 0xDEADBEEF);
    REQUIRE(mb.receive(&out, sizeof(out)) == sizeof(uint32_t));
    REQUIRE(out == 0xCAFEBABE);
}

static void producer_task(void *param) {
    auto *mb = reinterpret_cast<utkernel::message_buffer *>(param);
    for (int32_t i = 0; i < 10; ++i) {
        mb->send(&i, sizeof(i));
    }
}

TEST_CASE("message_buffer cross-task", "[msgbuf]") {
    utkernel::message_buffer mb(512, 64);

    utkernel::task::config cfg;
    cfg.name = "producer";
    cfg.stack_size = 4096;
    cfg.param = &mb;

    utkernel::task t(producer_task, cfg);
    int32_t sum = 0;
    for (int32_t i = 0; i < 10; ++i) {
        int32_t val = 0;
        mb.receive(&val, sizeof(val));
        sum += val;
    }
    REQUIRE(sum == 45);
}
