/**
 * string.cpp
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#include "common/string.hpp"

using namespace cassio;

bool cassio::streq(const char* a, const char* b) {
    u32 i = 0;
    while (a[i] != '\0' && b[i] != '\0') {
        if (a[i] != b[i]) return false;
        ++i;
    }
    return a[i] == b[i];
}

void cassio::strcpy(char* dst, const char* src, usize max) {
    usize i = 0;
    while (i < max - 1 && src[i] != '\0') {
        dst[i] = src[i];
        ++i;
    }
    dst[i] = '\0';
}

usize cassio::strlen(const char* str) {
    usize len = 0;
    while (str[len] != '\0') {
        ++len;
    }
    return len;
}
