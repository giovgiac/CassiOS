/**
 * test.hpp -- Standard test framework
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 * Shared between kernel and userspace tests. Output is routed through
 * a write callback set via test::init() at the start of each runner.
 *
 */

#ifndef STD_TEST_HPP
#define STD_TEST_HPP

#include <std/types.hpp>

namespace std {
namespace test {

using WriteFn = void (*)(const char* buf, u32 len);

struct TestNode {
    const char* name;
    void (*fn)(const char*, bool&);
    TestNode* next;
};

extern TestNode* test_list_head;

void init(WriteFn fn);
u32 run();
void fail(const char* name, const char* expr, const char* file, int line);
void fail_eq(const char* name, u32 expected, u32 got, const char* file, int line);

} // namespace test
} // namespace std

#define TEST(name)                                                                                 \
    static void test_fn_##name(const char* _test_name, bool& _test_failed);                        \
    static std::test::TestNode test_node_##name = {#name, test_fn_##name, nullptr};                \
    static void __attribute__((constructor)) test_register_##name() {                              \
        test_node_##name.next = std::test::test_list_head;                                         \
        std::test::test_list_head = &test_node_##name;                                             \
    }                                                                                              \
    static void test_fn_##name(const char* _test_name, bool& _test_failed)

#define ASSERT(expr)                                                                               \
    do {                                                                                           \
        if (!(expr)) {                                                                             \
            std::test::fail(_test_name, #expr, __FILE__, __LINE__);                                \
            _test_failed = true;                                                                   \
            return;                                                                                \
        }                                                                                          \
    } while (0)

#define ASSERT_EQ(a, b)                                                                            \
    do {                                                                                           \
        auto _a = (a);                                                                             \
        auto _b = (b);                                                                             \
        if (_a != _b) {                                                                            \
            std::test::fail_eq(_test_name, static_cast<std::u32>(_a), static_cast<std::u32>(_b),   \
                               __FILE__, __LINE__);                                                \
            _test_failed = true;                                                                   \
            return;                                                                                \
        }                                                                                          \
    } while (0)

#endif // STD_TEST_HPP
