/**
 * fmt.cpp -- Standard string formatting
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#include <std/fmt.hpp>

using namespace std;

// Put a single character into the buffer if there is room.
static usize put(char* buf, usize size, usize pos, char c) {
    if (pos < size - 1) {
        buf[pos] = c;
    }
    return pos + 1;
}

// Write an unsigned 32-bit value as decimal.
static usize put_dec(char* buf, usize size, usize pos, u32 value) {
    if (value == 0) {
        return put(buf, size, pos, '0');
    }

    char digits[10];
    usize count = 0;
    while (value > 0) {
        digits[count++] = '0' + (value % 10);
        value /= 10;
    }

    for (usize i = count; i > 0; --i) {
        pos = put(buf, size, pos, digits[i - 1]);
    }
    return pos;
}

// Write an unsigned 32-bit value as hex.
static usize put_hex(char* buf, usize size, usize pos, u32 value, bool upper) {
    if (value == 0) {
        return put(buf, size, pos, '0');
    }

    const char* chars = upper ? "0123456789ABCDEF" : "0123456789abcdef";

    char digits[8];
    usize count = 0;
    while (value > 0) {
        digits[count++] = chars[value & 0xF];
        value >>= 4;
    }

    for (usize i = count; i > 0; --i) {
        pos = put(buf, size, pos, digits[i - 1]);
    }
    return pos;
}

// Write a null-terminated string.
static usize put_str(char* buf, usize size, usize pos, const char* s) {
    if (s == nullptr) {
        s = "(null)";
    }
    while (*s != '\0') {
        pos = put(buf, size, pos, *s++);
    }
    return pos;
}

usize fmt::format(char* buf, usize size, const char* fmt, ...) {
    if (size == 0) return 0;

    __builtin_va_list args;
    __builtin_va_start(args, fmt);

    usize pos = 0;

    for (usize i = 0; fmt[i] != '\0'; ++i) {
        if (fmt[i] != '%') {
            pos = put(buf, size, pos, fmt[i]);
            continue;
        }

        ++i;
        if (fmt[i] == '\0') break;

        switch (fmt[i]) {
        case 'd': {
            i32 val = __builtin_va_arg(args, i32);
            if (val < 0) {
                pos = put(buf, size, pos, '-');
                val = -val;
            }
            pos = put_dec(buf, size, pos, static_cast<u32>(val));
            break;
        }
        case 'u': {
            u32 val = __builtin_va_arg(args, u32);
            pos = put_dec(buf, size, pos, val);
            break;
        }
        case 'x': {
            u32 val = __builtin_va_arg(args, u32);
            pos = put_hex(buf, size, pos, val, false);
            break;
        }
        case 'X': {
            u32 val = __builtin_va_arg(args, u32);
            pos = put_hex(buf, size, pos, val, true);
            break;
        }
        case 's': {
            const char* val = __builtin_va_arg(args, const char*);
            pos = put_str(buf, size, pos, val);
            break;
        }
        case 'c': {
            char val = static_cast<char>(__builtin_va_arg(args, int));
            pos = put(buf, size, pos, val);
            break;
        }
        case '%': {
            pos = put(buf, size, pos, '%');
            break;
        }
        default: {
            // Unknown specifier: output as-is.
            pos = put(buf, size, pos, '%');
            pos = put(buf, size, pos, fmt[i]);
            break;
        }
        }
    }

    __builtin_va_end(args);

    // Null-terminate.
    if (pos < size) {
        buf[pos] = '\0';
    } else {
        buf[size - 1] = '\0';
    }

    // Return characters written (excluding null terminator).
    return pos < size ? pos : size - 1;
}
