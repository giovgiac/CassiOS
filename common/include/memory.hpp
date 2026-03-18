/**
 * memory.hpp -- Standard memory functions (memcpy, memset, memmove, memcmp)
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 * Declared extern "C" so GCC can link implicit calls it generates
 * for struct copies, zero-initialization, and large assignments.
 *
 */

#ifndef COMMON_MEMORY_HPP_
#define COMMON_MEMORY_HPP_

#include <std/types.hpp>

extern "C" {

void* memcpy(void* dst, const void* src, std::usize n);
void* memmove(void* dst, const void* src, std::usize n);
void* memset(void* dst, int val, std::usize n);
int memcmp(const void* a, const void* b, std::usize n);

}

#endif // COMMON_MEMORY_HPP_
