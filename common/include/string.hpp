/**
 * string.hpp
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#ifndef COMMON_STRING_HPP_
#define COMMON_STRING_HPP_

#include <std/types.hpp>

namespace cassio {

bool streq(const char* a, const char* b);
void strcpy(char* dst, const char* src, std::usize max);
std::usize strlen(const char* str);
std::u32 strtou32(const char* str);

} // cassio

#endif // COMMON_STRING_HPP_
