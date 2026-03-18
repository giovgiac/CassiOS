/**
 * test.hpp -- unified test framework
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 * Shared between kernel and userspace tests. Output is routed through
 * a write callback set via test::init() at the start of each runner.
 *
 */

#ifndef COMMON_TEST_HPP_
#define COMMON_TEST_HPP_

#include <std/types.hpp>
#include <string.hpp>

namespace test {

using namespace cassio;

using WriteFn = void(*)(const char* buf, std::u32 len);

struct TestNode {
    const char* name;
    void (*fn)(const char*, bool&);
    TestNode* next;
};

inline TestNode* test_list_head = nullptr;
inline WriteFn write_fn = nullptr;

inline void init(WriteFn fn) {
    write_fn = fn;
}

inline void serial_puts(const char* s) {
    write_fn(s, strlen(s));
}

inline void serial_putchar(char c) {
    write_fn(&c, 1);
}

inline void serial_put_hex(std::u32 value) {
    const char* hex = "0123456789ABCDEF";
    serial_puts("0x");
    for (std::i32 i = 28; i >= 0; i -= 4) {
        serial_putchar(hex[(value >> i) & 0xF]);
    }
}

inline void serial_put_dec(std::u32 value) {
    if (value == 0) {
        serial_putchar('0');
        return;
    }
    char buf[12];
    std::i32 i = 0;
    while (value > 0) {
        buf[i++] = '0' + (value % 10);
        value /= 10;
    }
    for (std::i32 j = 0; j < i / 2; j++) {
        char tmp = buf[j];
        buf[j] = buf[i - 1 - j];
        buf[i - 1 - j] = tmp;
    }
    write_fn(buf, i);
}

inline void serial_put_location(const char* file, int line) {
    serial_puts(" at ");
    serial_puts(file);
    serial_putchar(':');
    serial_put_dec(static_cast<std::u32>(line));
}

inline std::u32 run() {
    std::u32 passed = 0, failed = 0;
    for (TestNode* t = test_list_head; t; t = t->next) {
        bool test_failed = false;
        t->fn(t->name, test_failed);
        if (!test_failed) {
            serial_puts("[PASS] ");
            serial_puts(t->name);
            serial_putchar('\n');
            passed++;
        } else {
            failed++;
        }
    }

    serial_puts("[DONE] ");
    serial_put_dec(passed);
    serial_puts(" passed, ");
    serial_put_dec(failed);
    serial_puts(" failed\n");

    return failed;
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
            test::serial_put_hex(static_cast<std::u32>(_a));                   \
            test::serial_puts(", got ");                                           \
            test::serial_put_hex(static_cast<std::u32>(_b));                   \
            test::serial_put_location(__FILE__, __LINE__);                         \
            test::serial_putchar('\n');                                            \
            _test_failed = true;                                                  \
            return;                                                               \
        }                                                                         \
    } while (0)

#endif // COMMON_TEST_HPP_
