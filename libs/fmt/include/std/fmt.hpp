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
//   {{  }}          literal '{' or '}'

#ifndef STD_FMT_HPP
#define STD_FMT_HPP

#include <std/str.hpp>
#include <std/types.hpp>

namespace std {
namespace fmt {

/**
 * @brief Type-erased format argument.
 *
 * Implicit constructors allow callers to pass any supported type
 * directly. Unsupported types produce a compile error (no matching
 * constructor).
 *
 */
struct Arg {
    enum Type : u8 { I32, U32, Str, Char };

    Type type;
    union {
        i32 asI32;
        u32 asU32;
        const char* asStr;
        char asChar;
    };

    Arg(i32 v) : type(I32), asI32(v) {}
    Arg(u32 v) : type(U32), asU32(v) {}
    Arg(const char* v) : type(Str), asStr(v) {}
    Arg(char v) : type(Char), asChar(v) {}
    Arg(const str::StringView& v) : type(Str), asStr(v.data()) {}
};

// Core formatter -- implemented in fmt.cpp.
usize formatImpl(char* buf, usize size, const char* fmt, const Arg* args, usize argCount);

// Public API -- thin template that builds the Arg array.
template <typename... Args>
usize format(char* buf, usize size, const char* fmt, const Args&... args) {
    if (size == 0) {
        return 0;
    }
    Arg arr[] = {Arg(args)...};
    return formatImpl(buf, size, fmt, arr, sizeof...(args));
}

// Zero-argument overload (no args to pack).
inline usize format(char* buf, usize size, const char* fmt) {
    if (size == 0) {
        return 0;
    }
    return formatImpl(buf, size, fmt, nullptr, 0);
}

} // namespace fmt
} // namespace std

#endif // STD_FMT_HPP
