/**
 * alloc.hpp -- placement new
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 * The kernel heap itself is Rust (kernel/src/memory/heap.rs).
 *
 */

#ifndef STD_ALLOC_HPP
#define STD_ALLOC_HPP

#include <std/types.hpp>

// Placement new operator.
inline void* operator new(std::usize, void* ptr) {
    return ptr;
}

#endif // STD_ALLOC_HPP
