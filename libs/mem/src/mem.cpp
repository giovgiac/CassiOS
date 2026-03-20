/**
 * mem.cpp -- Standard memory functions
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#include <std/mem.hpp>

using namespace std;

void* mem::copy(void* dst, const void* src, usize n) {
    // Copy 4 bytes at a time, then handle remainder.
    u32* d32 = static_cast<u32*>(dst);
    const u32* s32 = static_cast<const u32*>(src);
    usize words = n / 4;
    for (usize i = 0; i < words; i++) {
        d32[i] = s32[i];
    }
    u8* d = reinterpret_cast<u8*>(d32 + words);
    const u8* s = reinterpret_cast<const u8*>(s32 + words);
    for (usize i = 0; i < n % 4; i++) {
        d[i] = s[i];
    }
    return dst;
}

void* mem::move(void* dst, const void* src, usize n) {
    u8* d = static_cast<u8*>(dst);
    const u8* s = static_cast<const u8*>(src);
    if (d < s) {
        mem::copy(dst, src, n);
    } else if (d > s) {
        for (usize i = n; i > 0; i--) {
            d[i - 1] = s[i - 1];
        }
    }
    return dst;
}

void* mem::set(void* dst, int val, usize n) {
    u8 v = static_cast<u8>(val);
    u32 v32 = static_cast<u32>(v) | (static_cast<u32>(v) << 8) | (static_cast<u32>(v) << 16) |
              (static_cast<u32>(v) << 24);
    u32* d32 = static_cast<u32*>(dst);
    usize words = n / 4;
    for (usize i = 0; i < words; i++) {
        d32[i] = v32;
    }
    u8* d = reinterpret_cast<u8*>(d32 + words);
    for (usize i = 0; i < n % 4; i++) {
        d[i] = v;
    }
    return dst;
}

int mem::compare(const void* a, const void* b, usize n) {
    const u8* pa = static_cast<const u8*>(a);
    const u8* pb = static_cast<const u8*>(b);
    for (usize i = 0; i < n; i++) {
        if (pa[i] != pb[i]) {
            return pa[i] < pb[i] ? -1 : 1;
        }
    }
    return 0;
}

// Provide extern "C" symbols for GCC's implicit calls (struct copies,
// zero-initialization, large assignments).
extern "C" void* memcpy(void* dst, const void* src, usize n) {
    return mem::copy(dst, src, n);
}

extern "C" void* memmove(void* dst, const void* src, usize n) {
    return mem::move(dst, src, n);
}

extern "C" void* memset(void* dst, int val, usize n) {
    return mem::set(dst, val, n);
}

extern "C" int memcmp(const void* a, const void* b, usize n) {
    return mem::compare(a, b, n);
}
