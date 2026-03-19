/**
 * str.cpp -- Standard string functions
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#include <std/str.hpp>

using namespace std;

bool str::eq(const char* a, const char* b) {
    u32 i = 0;
    while (a[i] != '\0' && b[i] != '\0') {
        if (a[i] != b[i]) return false;
        ++i;
    }
    return a[i] == b[i];
}

void str::copy(char* dst, const char* src, usize max) {
    usize i = 0;
    while (i < max - 1 && src[i] != '\0') {
        dst[i] = src[i];
        ++i;
    }
    dst[i] = '\0';
}

usize str::len(const char* s) {
    usize l = 0;
    while (s[l] != '\0') {
        ++l;
    }
    return l;
}

u32 str::to_u32(const char* s) {
    u32 result = 0;
    for (usize i = 0; s[i] != '\0'; ++i) {
        if (s[i] >= '0' && s[i] <= '9') {
            result = result * 10 + (s[i] - '0');
        } else {
            break;
        }
    }
    return result;
}
