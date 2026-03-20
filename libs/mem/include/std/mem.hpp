/**
 * mem.hpp -- Standard memory functions (copy, move, set, compare)
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#ifndef STD_MEM_HPP
#define STD_MEM_HPP

#include <std/types.hpp>

namespace std {
namespace mem {

void* copy(void* dst, const void* src, usize n);
void* move(void* dst, const void* src, usize n);
void* set(void* dst, int val, usize n);
int compare(const void* a, const void* b, usize n);

} // namespace mem
} // namespace std

#endif // STD_MEM_HPP
