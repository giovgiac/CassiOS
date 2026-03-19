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

