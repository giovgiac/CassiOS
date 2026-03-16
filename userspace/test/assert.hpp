/**
 * assert.hpp -- userspace test framework
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 * Provides TEST(), ASSERT(), and ASSERT_EQ() macros for userspace
 * integration tests. Output goes to serial (fd=2) via System::write().
 *
 */

#ifndef USERSPACE_TEST_ASSERT_HPP_
#define USERSPACE_TEST_ASSERT_HPP_

#include <types.hpp>
#include <string.hpp>
#include <system.hpp>

namespace test {

using namespace cassio;

struct TestNode {
    const char* name;
    void (*fn)(const char*, bool&);
    TestNode* next;
};

inline TestNode* test_list_head = nullptr;

inline void serial_puts(const char* s) {
    System::write(2, s, strlen(s));
}

inline void serial_putchar(char c) {
    System::write(2, &c, 1);
}

inline void serial_put_hex(u32 value) {
    const char* hex = "0123456789ABCDEF";
    serial_puts("0x");
    for (i32 i = 28; i >= 0; i -= 4) {
        serial_putchar(hex[(value >> i) & 0xF]);
    }
}

inline void serial_put_dec(u32 value) {
    if (value == 0) {
        serial_putchar('0');
        return;
    }
    char buf[12];
    i32 i = 0;
    while (value > 0) {
        buf[i++] = '0' + (value % 10);
        value /= 10;
    }
    for (i32 j = 0; j < i / 2; j++) {
        char tmp = buf[j];
        buf[j] = buf[i - 1 - j];
        buf[i - 1 - j] = tmp;
    }
    System::write(2, buf, i);
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
            test::serial_puts("[FAIL] ");                                          \
            test::serial_puts(_test_name);                                         \
            test::serial_puts(": assertion failed: \"" #expr "\"");               \
            test::serial_put_location(__FILE__, __LINE__);                         \
            test::serial_putchar('\n');                                            \
            _test_failed = true;                                                  \
            return;                                                               \
        }                                                                         \
    } while (0)

#define ASSERT_EQ(a, b)                                                           \
    do {                                                                          \
        auto _a = (a);                                                            \
        auto _b = (b);                                                            \
        if (_a != _b) {                                                           \
            test::serial_puts("[FAIL] ");                                          \
            test::serial_puts(_test_name);                                         \
            test::serial_puts(": expected ");                                      \
            test::serial_put_hex(static_cast<cassio::u32>(_a));                   \
            test::serial_puts(", got ");                                           \
            test::serial_put_hex(static_cast<cassio::u32>(_b));                   \
            test::serial_put_location(__FILE__, __LINE__);                         \
            test::serial_putchar('\n');                                            \
            _test_failed = true;                                                  \
            return;                                                               \
        }                                                                         \
    } while (0)

#endif // USERSPACE_TEST_ASSERT_HPP_
