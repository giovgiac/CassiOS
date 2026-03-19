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

#include <std/test.hpp>
#include <std/os.hpp>
#include <std/heap.hpp>

using namespace std;

typedef void (*ctor)();
extern "C" ctor start_ctors;
extern "C" ctor end_ctors;

static void ctors() {
    for (ctor* ct = &start_ctors; ct != &end_ctors; ++ct) {
        (*ct)();
    }
}

static void userspace_write(const char* buf, u32 len) {
    os::write(2, buf, len);
}

extern "C" void _start() {
    ctors();

    test::init(userspace_write);
    u32 failed = test::run();

    os::exit(failed > 0 ? 1 : 0);
}
