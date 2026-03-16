#ifndef TESTS_TEST_HPP_
#define TESTS_TEST_HPP_

#include <common/types.hpp>
#include <hardware/serial.hpp>

namespace test {

using namespace cassio;
using namespace cassio::hardware;

struct TestNode {
    const char* name;
    void (*fn)(const char*, bool&);
    TestNode* next;
};

inline TestNode* test_list_head = nullptr;

inline void serial_put_hex(u32 value) {
    Serial& s = COM1::getSerial();
    const char* hex = "0123456789ABCDEF";
    s.puts("0x");
    for (i32 i = 28; i >= 0; i -= 4) {
        s.putchar(hex[(value >> i) & 0xF]);
    }
}

inline void serial_put_location(const char* file, int line) {
    Serial& s = COM1::getSerial();
    s.puts(" at ");
    s.puts(file);
    s.putchar(':');
    s.put_dec(static_cast<u32>(line));
}

} // test

#define TEST(name)                                                               \
    static void test_fn_##name(const char* _test_name, bool& _test_failed);      \
    static test::TestNode test_node_##name = { #name, test_fn_##name, nullptr };  \
    static void __attribute__((constructor)) test_register_##name() {             \
        test_node_##name.next = test::test_list_head;                             \
        test::test_list_head = &test_node_##name;                                 \
    }                                                                             \
    static void test_fn_##name(const char* _test_name, bool& _test_failed)

#define ASSERT(expr)                                                              \
    do {                                                                          \
        if (!(expr)) {                                                            \
            cassio::hardware::Serial& _s = cassio::hardware::COM1::getSerial(); \
            _s.puts("[FAIL] ");                                                   \
            _s.puts(_test_name);                                                  \
            _s.puts(": assertion failed: \"" #expr "\"");                         \
            test::serial_put_location(__FILE__, __LINE__);                         \
            _s.putchar('\n');                                                      \
            _test_failed = true;                                                  \
            return;                                                               \
        }                                                                         \
    } while (0)

#define ASSERT_EQ(a, b)                                                           \
    do {                                                                          \
        auto _a = (a);                                                            \
        auto _b = (b);                                                            \
        if (_a != _b) {                                                           \
            cassio::hardware::Serial& _s = cassio::hardware::COM1::getSerial(); \
            _s.puts("[FAIL] ");                                                   \
            _s.puts(_test_name);                                                  \
            _s.puts(": expected ");                                               \
            test::serial_put_hex(static_cast<cassio::u32>(_a));                   \
            _s.puts(", got ");                                                    \
            test::serial_put_hex(static_cast<cassio::u32>(_b));                   \
            test::serial_put_location(__FILE__, __LINE__);                         \
            _s.putchar('\n');                                                      \
            _test_failed = true;                                                  \
            return;                                                               \
        }                                                                         \
    } while (0)

#endif // TESTS_TEST_HPP_
