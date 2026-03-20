// fmt.hpp -- Type-safe string formatting with {} placeholders
//
// Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file.
//
// Format syntax:
//   {}              default (decimal for integers, string for char*/StringView)
//   {:x}  {:X}     hex (lower/upper)
//   {:5}           right-align, width 5
//   {:<5}          left-align, width 5
//   {:05}          zero-pad, width 5
//   {:08x}         zero-pad hex, width 8
//   {{              literal '{'

#ifndef STD_FMT_HPP
#define STD_FMT_HPP

#include <std/str.hpp>
#include <std/types.hpp>

namespace std {
namespace fmt {
namespace detail {

// -- Output helpers --

inline usize put(char* buf, usize size, usize pos, char c) {
    if (pos < size - 1) {
        buf[pos] = c;
    }
    return pos + 1;
}

inline usize putPad(char* buf, usize size, usize pos, char c, u32 count) {
    for (u32 p = 0; p < count; ++p) {
        pos = put(buf, size, pos, c);
    }
    return pos;
}

// -- Number-to-string helpers --

inline usize fmtDec(char* tmp, u32 value) {
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

inline usize fmtHex(char* tmp, u32 value, bool upper) {
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

// -- Format spec parsed from {:...} --

struct Spec {
    u32 width;
    bool leftAlign;
    bool zeroPad;
    bool hex;
    bool upperHex;
};

inline Spec parseSpec(const char* fmt, usize& i) {
    Spec s = {0, false, false, false, false};

    // Skip past ':'.
    if (fmt[i] == ':') {
        ++i;
    }

    // Parse flags.
    if (fmt[i] == '<') {
        s.leftAlign = true;
        ++i;
    }
    if (fmt[i] == '0') {
        s.zeroPad = true;
        ++i;
    }

    // Parse width.
    while (fmt[i] >= '0' && fmt[i] <= '9') {
        s.width = s.width * 10 + (fmt[i] - '0');
        ++i;
    }

    // Parse hex.
    if (fmt[i] == 'x') {
        s.hex = true;
        ++i;
    } else if (fmt[i] == 'X') {
        s.hex = true;
        s.upperHex = true;
        ++i;
    }

    // Skip closing '}'.
    if (fmt[i] == '}') {
        ++i;
    }

    // Left-align overrides zero-pad.
    if (s.leftAlign) {
        s.zeroPad = false;
    }

    return s;
}

// -- Padded output --

inline usize putPadded(char* buf, usize size, usize pos, const char* tmp, usize len, const Spec& s,
                       char sign = 0) {
    char padChar = s.zeroPad ? '0' : ' ';
    usize total = len + (sign ? 1 : 0);
    u32 padding = (s.width > total) ? s.width - total : 0;

    if (!s.leftAlign) {
        if (padChar == '0' && sign) {
            pos = put(buf, size, pos, sign);
            sign = 0;
        }
        pos = putPad(buf, size, pos, padChar, padding);
    }

    if (sign) {
        pos = put(buf, size, pos, sign);
    }

    for (usize j = 0; j < len; ++j) {
        pos = put(buf, size, pos, tmp[j]);
    }

    if (s.leftAlign) {
        pos = putPad(buf, size, pos, ' ', padding);
    }

    return pos;
}

// -- Type-specific formatArg overloads --

inline usize formatArg(char* buf, usize size, usize pos, u32 value, const Spec& s) {
    char tmp[12];
    usize len = s.hex ? fmtHex(tmp, value, s.upperHex) : fmtDec(tmp, value);
    return putPadded(buf, size, pos, tmp, len, s);
}

inline usize formatArg(char* buf, usize size, usize pos, i32 value, const Spec& s) {
    char tmp[12];
    char sign = 0;
    u32 absVal;
    if (value < 0) {
        sign = '-';
        absVal = static_cast<u32>(-value);
    } else {
        absVal = static_cast<u32>(value);
    }
    usize len = s.hex ? fmtHex(tmp, absVal, s.upperHex) : fmtDec(tmp, absVal);
    return putPadded(buf, size, pos, tmp, len, s, sign);
}

inline usize formatArg(char* buf, usize size, usize pos, const char* value, const Spec& s) {
    if (value == nullptr) {
        value = "(null)";
    }
    usize len = 0;
    while (value[len] != '\0') {
        ++len;
    }

    u32 padding = (s.width > len) ? s.width - len : 0;

    if (!s.leftAlign) {
        pos = putPad(buf, size, pos, ' ', padding);
    }
    for (usize j = 0; j < len; ++j) {
        pos = put(buf, size, pos, value[j]);
    }
    if (s.leftAlign) {
        pos = putPad(buf, size, pos, ' ', padding);
    }
    return pos;
}

inline usize formatArg(char* buf, usize size, usize pos, char value, const Spec& s) {
    (void)s;
    return put(buf, size, pos, value);
}

inline usize formatArg(char* buf, usize size, usize pos, const str::StringView& value,
                       const Spec& s) {
    usize len = value.length();
    u32 padding = (s.width > len) ? s.width - len : 0;

    if (!s.leftAlign) {
        pos = putPad(buf, size, pos, ' ', padding);
    }
    for (usize j = 0; j < len; ++j) {
        pos = put(buf, size, pos, value[j]);
    }
    if (s.leftAlign) {
        pos = putPad(buf, size, pos, ' ', padding);
    }
    return pos;
}

// -- Copy remaining format string (no more arguments) --

inline usize copyTail(char* buf, usize size, usize pos, const char* fmt) {
    for (usize i = 0; fmt[i] != '\0'; ++i) {
        if (fmt[i] == '{' && fmt[i + 1] == '{') {
            pos = put(buf, size, pos, '{');
            ++i;
            continue;
        }
        if (fmt[i] == '}' && fmt[i + 1] == '}') {
            pos = put(buf, size, pos, '}');
            ++i;
            continue;
        }
        pos = put(buf, size, pos, fmt[i]);
    }
    return pos;
}

// -- Find next {} placeholder and format one argument --

template <typename T, typename... Rest>
usize formatImpl(char* buf, usize size, usize pos, const char* fmt, const T& arg,
                 const Rest&... rest);

// Base case: no arguments left, copy remaining format string.
inline usize formatImpl(char* buf, usize size, usize pos, const char* fmt) {
    return copyTail(buf, size, pos, fmt);
}

// Recursive case: find next {}, format one argument, recurse.
template <typename T, typename... Rest>
usize formatImpl(char* buf, usize size, usize pos, const char* fmt, const T& arg,
                 const Rest&... rest) {
    for (usize i = 0; fmt[i] != '\0'; ++i) {
        if (fmt[i] == '{' && fmt[i + 1] == '{') {
            pos = put(buf, size, pos, '{');
            ++i;
            continue;
        }
        if (fmt[i] == '}' && fmt[i + 1] == '}') {
            pos = put(buf, size, pos, '}');
            ++i;
            continue;
        }

        // Found a placeholder.
        if (fmt[i] == '{') {
            ++i; // skip '{'
            Spec s = parseSpec(fmt, i);
            pos = formatArg(buf, size, pos, arg, s);
            // Recurse with remaining format string and arguments.
            return formatImpl(buf, size, pos, fmt + i, rest...);
        }

        pos = put(buf, size, pos, fmt[i]);
    }

    // Format string ended before all arguments were consumed. Ignore extras.
    return pos;
}

} // namespace detail

// -- Public API --

template <typename... Args>
usize format(char* buf, usize size, const char* fmt, const Args&... args) {
    if (size == 0) {
        return 0;
    }

    usize pos = detail::formatImpl(buf, size, 0, fmt, args...);

    if (pos < size) {
        buf[pos] = '\0';
    } else {
        buf[size - 1] = '\0';
    }

    return pos < size ? pos : size - 1;
}

} // namespace fmt
} // namespace std

#endif // STD_FMT_HPP
