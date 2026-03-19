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

// Write count copies of a padding character.
static usize put_pad(char* buf, usize size, usize pos, char c, u32 count) {
    for (u32 p = 0; p < count; ++p) {
        pos = put(buf, size, pos, c);
    }
    return pos;
}

// Format an unsigned 32-bit value as decimal into tmp, return length.
static usize fmt_dec(char* tmp, u32 value) {
    if (value == 0) {
        tmp[0] = '0';
        return 1;
    }
    char digits[10];
    usize count = 0;
    while (value > 0) {
        digits[count++] = '0' + (value % 10);
        value /= 10;
    }
    for (usize i = 0; i < count; ++i) {
        tmp[i] = digits[count - 1 - i];
    }
    return count;
}

// Format an unsigned 32-bit value as hex into tmp, return length.
static usize fmt_hex(char* tmp, u32 value, bool upper) {
    if (value == 0) {
        tmp[0] = '0';
        return 1;
    }
    const char* chars = upper ? "0123456789ABCDEF" : "0123456789abcdef";
    char digits[8];
    usize count = 0;
    while (value > 0) {
        digits[count++] = chars[value & 0xF];
        value >>= 4;
    }
    for (usize i = 0; i < count; ++i) {
        tmp[i] = digits[count - 1 - i];
    }
    return count;
}

// Write a formatted value with padding and optional sign.
static usize put_padded(char* buf, usize size, usize pos,
                        const char* tmp, usize len,
                        u32 width, bool left_align, char pad_char,
                        char sign = 0) {
    usize total = len + (sign ? 1 : 0);
    u32 padding = (width > total) ? width - total : 0;

    if (!left_align) {
        if (pad_char == '0' && sign) {
            pos = put(buf, size, pos, sign);
            sign = 0;
        }
        pos = put_pad(buf, size, pos, pad_char, padding);
    }

    if (sign) {
        pos = put(buf, size, pos, sign);
    }

    for (usize j = 0; j < len; ++j) {
        pos = put(buf, size, pos, tmp[j]);
    }

    if (left_align) {
        pos = put_pad(buf, size, pos, ' ', padding);
    }

    return pos;
}

// Write a string with padding.
static usize put_str_padded(char* buf, usize size, usize pos,
                            const char* s, u32 width, bool left_align) {
    if (s == nullptr) s = "(null)";

    usize len = 0;
    while (s[len] != '\0') ++len;

    u32 padding = (width > len) ? width - len : 0;

    if (!left_align) {
        pos = put_pad(buf, size, pos, ' ', padding);
    }

    for (usize j = 0; j < len; ++j) {
        pos = put(buf, size, pos, s[j]);
    }

    if (left_align) {
        pos = put_pad(buf, size, pos, ' ', padding);
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

        // Parse flags.
        bool left_align = false;
        bool zero_pad = false;
        while (fmt[i] == '-' || fmt[i] == '0') {
            if (fmt[i] == '-') left_align = true;
            if (fmt[i] == '0') zero_pad = true;
            ++i;
        }

        // Parse width.
        u32 width = 0;
        while (fmt[i] >= '0' && fmt[i] <= '9') {
            width = width * 10 + (fmt[i] - '0');
            ++i;
        }

        if (fmt[i] == '\0') break;

        // Left-align overrides zero-pad.
        char pad_char = (zero_pad && !left_align) ? '0' : ' ';

        char tmp[12];

        switch (fmt[i]) {
        case 'd': {
            i32 val = __builtin_va_arg(args, i32);
            char sign = 0;
            u32 abs_val;
            if (val < 0) {
                sign = '-';
                abs_val = static_cast<u32>(-val);
            } else {
                abs_val = static_cast<u32>(val);
            }
            usize len = fmt_dec(tmp, abs_val);
            pos = put_padded(buf, size, pos, tmp, len, width, left_align, pad_char, sign);
            break;
        }
        case 'u': {
            u32 val = __builtin_va_arg(args, u32);
            usize len = fmt_dec(tmp, val);
            pos = put_padded(buf, size, pos, tmp, len, width, left_align, pad_char);
            break;
        }
        case 'x': {
            u32 val = __builtin_va_arg(args, u32);
            usize len = fmt_hex(tmp, val, false);
            pos = put_padded(buf, size, pos, tmp, len, width, left_align, pad_char);
            break;
        }
        case 'X': {
            u32 val = __builtin_va_arg(args, u32);
            usize len = fmt_hex(tmp, val, true);
            pos = put_padded(buf, size, pos, tmp, len, width, left_align, pad_char);
            break;
        }
        case 's': {
            const char* val = __builtin_va_arg(args, const char*);
            pos = put_str_padded(buf, size, pos, val, width, left_align);
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
            pos = put(buf, size, pos, '%');
            pos = put(buf, size, pos, fmt[i]);
            break;
        }
        }
    }

    __builtin_va_end(args);

    if (pos < size) {
        buf[pos] = '\0';
    } else {
        buf[size - 1] = '\0';
    }

    return pos < size ? pos : size - 1;
}
