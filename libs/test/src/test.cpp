/**
 * test.cpp -- Standard test framework
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#include <std/fmt.hpp>
#include <std/test.hpp>

using namespace std;

test::TestNode* test::test_list_head = nullptr;

static test::WriteFn write_fn = nullptr;

static void puts(const char* s) {
    u32 len = 0;
    while (s[len] != '\0')
        ++len;
    write_fn(s, len);
}

void test::init(WriteFn fn) {
    write_fn = fn;
}

u32 test::run() {
    u32 passed = 0, failed = 0;
    for (TestNode* t = test_list_head; t; t = t->next) {
        bool test_failed = false;
        t->fn(t->name, test_failed);
        if (!test_failed) {
            puts("[PASS] ");
            puts(t->name);
            write_fn("\n", 1);
            passed++;
        } else {
            failed++;
        }
    }

    char buf[32];
    fmt::format(buf, sizeof(buf), "[DONE] %u passed, %u failed\n", passed, failed);
    puts(buf);

    return failed;
}

void test::fail(const char* name, const char* expr, const char* file, int line) {
    char buf[128];
    fmt::format(buf, sizeof(buf), "[FAIL] %s: assertion failed: \"%s\" at %s:%d\n", name, expr,
                file, line);
    puts(buf);
}

void test::fail_eq(const char* name, u32 expected, u32 got, const char* file, int line) {
    char buf[128];
    fmt::format(buf, sizeof(buf), "[FAIL] %s: expected 0x%X, got 0x%X at %s:%d\n", name, expected,
                got, file, line);
    puts(buf);
}
