/**
 * str.cpp -- StringView and string utilities
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#include <std/str.hpp>

using namespace std;

// -- StringView --

str::StringView::StringView(const char* s) : ptr(s), len(0) {
    if (s) {
        while (s[len] != '\0') {
            ++len;
        }
    }
}

bool str::StringView::operator==(const StringView& other) const {
    if (len != other.len) return false;
    for (usize i = 0; i < len; i++) {
        if (ptr[i] != other.ptr[i]) return false;
    }
    return true;
}

bool str::StringView::operator!=(const StringView& other) const {
    return !(*this == other);
}

i32 str::StringView::compare(const StringView& other) const {
    usize n = len < other.len ? len : other.len;
    for (usize i = 0; i < n; i++) {
        if (ptr[i] < other.ptr[i]) return -1;
        if (ptr[i] > other.ptr[i]) return 1;
    }
    if (len < other.len) return -1;
    if (len > other.len) return 1;
    return 0;
}

bool str::StringView::startsWith(const StringView& prefix) const {
    if (prefix.len > len) return false;
    for (usize i = 0; i < prefix.len; i++) {
        if (ptr[i] != prefix.ptr[i]) return false;
    }
    return true;
}

bool str::StringView::endsWith(const StringView& suffix) const {
    if (suffix.len > len) return false;
    usize offset = len - suffix.len;
    for (usize i = 0; i < suffix.len; i++) {
        if (ptr[offset + i] != suffix.ptr[i]) return false;
    }
    return true;
}

bool str::StringView::contains(char c) const {
    return indexOf(c) >= 0;
}

isize str::StringView::indexOf(char c) const {
    for (usize i = 0; i < len; i++) {
        if (ptr[i] == c) return static_cast<isize>(i);
    }
    return -1;
}

str::StringView str::StringView::substr(usize pos) const {
    if (pos >= len) return StringView();
    return StringView(ptr + pos, len - pos);
}

str::StringView str::StringView::substr(usize pos, usize count) const {
    if (pos >= len) return StringView();
    usize remaining = len - pos;
    usize n = count < remaining ? count : remaining;
    return StringView(ptr + pos, n);
}

void str::StringView::copyTo(char* dst, usize max) const {
    if (max == 0) return;
    usize n = len < max - 1 ? len : max - 1;
    for (usize i = 0; i < n; i++) {
        dst[i] = ptr[i];
    }
    dst[n] = '\0';
}

template <>
u32 str::StringView::to<u32>() const {
    u32 result = 0;
    for (usize i = 0; i < len; i++) {
        if (ptr[i] >= '0' && ptr[i] <= '9') {
            result = result * 10 + static_cast<u32>(ptr[i] - '0');
        } else {
            break;
        }
    }
    return result;
}

// -- Legacy free functions --

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
