/**
 * main.cpp -- userspace test runner
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 * Entry point for userspace integration tests. Runs all registered
 * tests, reports results to serial, and exits via QEMU debug port.
 *
 */

#include "assert.hpp"

using namespace cassio;

typedef void (*ctor)();
extern "C" ctor start_ctors;
extern "C" ctor end_ctors;

static void ctors() {
    for (ctor* ct = &start_ctors; ct != &end_ctors; ++ct) {
        (*ct)();
    }
}

extern "C" void _start() {
    ctors();

    u32 passed = 0, failed = 0;
    for (test::TestNode* t = test::test_list_head; t; t = t->next) {
        bool test_failed = false;
        t->fn(t->name, test_failed);
        if (!test_failed) {
            test::serial_puts("[PASS] ");
            test::serial_puts(t->name);
            test::serial_putchar('\n');
            passed++;
        } else {
            failed++;
        }
    }

    test::serial_puts("[DONE] ");
    test::serial_put_dec(passed);
    test::serial_puts(" passed, ");
    test::serial_put_dec(failed);
    test::serial_puts(" failed\n");

    System::exit(failed > 0 ? 1 : 0);
}
