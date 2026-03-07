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
    const char* hex = "0123456789ABCDEF";
    serial_puts("0x");
    for (i32 i = 28; i >= 0; i -= 4) {
        serial_putchar(hex[(value >> i) & 0xF]);
    }
}

inline void serial_put_location(const char* file, int line) {
    serial_puts(" at ");
    serial_puts(file);
    serial_putchar(':');
    serial_put_dec(static_cast<u32>(line));
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
            cassio::hardware::serial_puts("[FAIL] ");                             \
            cassio::hardware::serial_puts(_test_name);                            \
            cassio::hardware::serial_puts(": assertion failed: \"" #expr "\"");   \
            test::serial_put_location(__FILE__, __LINE__);                         \
            cassio::hardware::serial_putchar('\n');                                \
            _test_failed = true;                                                  \
            return;                                                               \
        }                                                                         \
    } while (0)

#define ASSERT_EQ(a, b)                                                           \
    do {                                                                          \
        auto _a = (a);                                                            \
        auto _b = (b);                                                            \
        if (_a != _b) {                                                           \
            cassio::hardware::serial_puts("[FAIL] ");                             \
            cassio::hardware::serial_puts(_test_name);                            \
            cassio::hardware::serial_puts(": expected ");                         \
            test::serial_put_hex(static_cast<cassio::u32>(_a));                   \
            cassio::hardware::serial_puts(", got ");                              \
            test::serial_put_hex(static_cast<cassio::u32>(_b));                   \
            test::serial_put_location(__FILE__, __LINE__);                         \
            cassio::hardware::serial_putchar('\n');                                \
            _test_failed = true;                                                  \
            return;                                                               \
        }                                                                         \
    } while (0)

#endif // TESTS_TEST_HPP_
