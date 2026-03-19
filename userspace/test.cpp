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

// --- Userspace heap tests ---

TEST(userheap_alloc_returns_non_null) {
    void* ptr = heap::alloc(64);
    ASSERT(ptr != nullptr);
    heap::free(ptr);
}

TEST(userheap_alloc_zero_returns_null) {
    void* ptr = heap::alloc(0);
    ASSERT(ptr == nullptr);
}

TEST(userheap_alloc_different_addresses) {
    void* a = heap::alloc(32);
    void* b = heap::alloc(32);
    ASSERT(a != nullptr);
    ASSERT(b != nullptr);
    ASSERT(a != b);
    heap::free(b);
    heap::free(a);
}

TEST(userheap_free_and_realloc) {
    void* a = heap::alloc(64);
    ASSERT(a != nullptr);
    heap::free(a);
    void* b = heap::alloc(64);
    ASSERT(b != nullptr);
    ASSERT_EQ((u32)a, (u32)b);
    heap::free(b);
}

TEST(userheap_write_and_read) {
    u32* arr = (u32*)heap::alloc(10 * sizeof(u32));
    ASSERT(arr != nullptr);
    for (u32 i = 0; i < 10; i++) {
        arr[i] = i * 7;
    }
    for (u32 i = 0; i < 10; i++) {
        ASSERT_EQ(arr[i], i * 7);
    }
    heap::free(arr);
}

extern "C" void _start() {
    ctors();

    test::init(userspace_write);
    u32 failed = test::run();

    os::exit(failed > 0 ? 1 : 0);
}
