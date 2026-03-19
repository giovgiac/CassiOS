/**
 * str.hpp -- Standard string functions (eq, copy, len, to_u32)
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#ifndef STD_STR_HPP
#define STD_STR_HPP

#include <std/types.hpp>

namespace std {
namespace str {

bool eq(const char* a, const char* b);
void copy(char* dst, const char* src, usize max);
usize len(const char* s);
u32 to_u32(const char* s);

}
}

#endif // STD_STR_HPP
