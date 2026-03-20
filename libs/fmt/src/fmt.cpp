/**
 * fmt.cpp -- Format engine: spec parsing, type dispatch, output
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#include <std/fmt.hpp>

using namespace std;

// -- Format spec parsed from {:...} --

struct Spec {
    u32 width;
    bool leftAlign;
    bool zeroPad;
    bool hex;
    bool upperHex;
};

// -- Output helpers --

static usize put(char* buf, usize size, usize pos, char c) {
    if (pos < size - 1) {
        buf[pos] = c;
    }
    return pos + 1;
}

static usize putPad(char* buf, usize size, usize pos, char c, u32 count) {
    for (u32 p = 0; p < count; ++p) {
        pos = put(buf, size, pos, c);
    }
    return pos;
}

// -- Number-to-string helpers --

static usize fmtDec(char* tmp, u32 value) {
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

static usize fmtHex(char* tmp, u32 value, bool upper) {
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

static Spec parseSpec(const char* fmt, usize& i) {
    Spec s = {0, false, false, false, false};

    if (fmt[i] == ':') {
        ++i;
    }

    if (fmt[i] == '<') {
        s.leftAlign = true;
        ++i;
    }
    if (fmt[i] == '0') {
        s.zeroPad = true;
        ++i;
    }

    while (fmt[i] >= '0' && fmt[i] <= '9') {
        s.width = s.width * 10 + (fmt[i] - '0');
        ++i;
    }

    if (fmt[i] == 'x') {
        s.hex = true;
        ++i;
    } else if (fmt[i] == 'X') {
        s.hex = true;
        s.upperHex = true;
        ++i;
    }

    if (fmt[i] == '}') {
        ++i;
    }

    if (s.leftAlign) {
        s.zeroPad = false;
    }

    return s;
}

// -- Padded output --

static usize putPadded(char* buf, usize size, usize pos, const char* data, usize len,
                       const Spec& s, char sign = 0) {
    char padChar = (s.zeroPad && !s.leftAlign) ? '0' : ' ';
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
        pos = put(buf, size, pos, data[j]);
    }

    if (s.leftAlign) {
        pos = putPad(buf, size, pos, ' ', padding);
    }

    return pos;
}

// -- Type-specific formatters --

static usize fmtU32(char* buf, usize size, usize pos, u32 value, const Spec& s) {
    char tmp[12];
    usize len = s.hex ? fmtHex(tmp, value, s.upperHex) : fmtDec(tmp, value);
    return putPadded(buf, size, pos, tmp, len, s);
}

static usize fmtI32(char* buf, usize size, usize pos, i32 value, const Spec& s) {
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

static usize fmtStr(char* buf, usize size, usize pos, const char* value, const Spec& s) {
    if (value == nullptr) {
        value = "(null)";
    }
    usize len = 0;
    while (value[len] != '\0') {
        ++len;
    }
    return putPadded(buf, size, pos, value, len, s);
}

static usize fmtSView(char* buf, usize size, usize pos, const char* data, usize len,
                       const Spec& s) {
    if (data == nullptr) {
        return fmtStr(buf, size, pos, nullptr, s);
    }
    return putPadded(buf, size, pos, data, len, s);
}

// -- Type dispatch --

static usize formatArg(char* buf, usize size, usize pos, const fmt::Arg& arg, const Spec& s) {
    switch (arg.type) {
    case fmt::Arg::I32:
        return fmtI32(buf, size, pos, arg.asI32, s);
    case fmt::Arg::U32:
        return fmtU32(buf, size, pos, arg.asU32, s);
    case fmt::Arg::Str:
        return fmtStr(buf, size, pos, arg.asStr, s);
    case fmt::Arg::Char:
        return put(buf, size, pos, arg.asChar);
    case fmt::Arg::SView:
        return fmtSView(buf, size, pos, arg.asSView.ptr, arg.asSView.len, s);
    }
    return pos;
}

// -- Core formatter --

usize fmt::formatImpl(char* buf, usize size, const char* fmt, const Arg* args, usize argCount) {
    usize pos = 0;
    usize argIdx = 0;
    usize i = 0;

    while (fmt[i] != '\0') {
        if (fmt[i] == '{' && fmt[i + 1] == '{') {
            pos = put(buf, size, pos, '{');
            i += 2;
            continue;
        }
        if (fmt[i] == '}' && fmt[i + 1] == '}') {
            pos = put(buf, size, pos, '}');
            i += 2;
            continue;
        }

        if (fmt[i] == '{' && argIdx < argCount) {
            ++i; // skip '{'
            Spec s = parseSpec(fmt, i);
            pos = formatArg(buf, size, pos, args[argIdx], s);
            ++argIdx;
            // parseSpec already advanced i past '}'
            continue;
        }

        pos = put(buf, size, pos, fmt[i]);
        ++i;
    }

    if (pos < size) {
        buf[pos] = '\0';
    } else {
        buf[size - 1] = '\0';
    }

    return pos < size ? pos : size - 1;
}
