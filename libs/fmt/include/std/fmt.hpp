/**
 * fmt.hpp -- Standard string formatting (sprintf-like)
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#ifndef STD_FMT_HPP
#define STD_FMT_HPP

#include <std/types.hpp>

namespace std {
namespace fmt {

/**
 * Format a string into a buffer, similar to snprintf.
 *
 * Format: %[flags][width]specifier
 *
 * Flags:
 *   -   Left-align within the field width (default is right-align)
 *   0   Pad with zeros instead of spaces (ignored with -)
 *
 * Width:
 *   A decimal number specifying the minimum field width.
 *
 * Specifiers:
 *   %d  - signed 32-bit decimal (i32)
 *   %u  - unsigned 32-bit decimal (u32)
 *   %x  - unsigned 32-bit lowercase hex (u32)
 *   %X  - unsigned 32-bit uppercase hex (u32)
 *   %s  - null-terminated string (const char*)
 *   %c  - single character (char, passed as int)
 *   %%  - literal '%'
 *
 * Returns the number of characters written, excluding the null terminator.
 * Always null-terminates the output if size > 0.
 */
usize format(char* buf, usize size, const char* fmt, ...);

}
}

#endif // STD_FMT_HPP
