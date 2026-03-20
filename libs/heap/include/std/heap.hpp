/**
 * heap.hpp -- userspace heap with operator new/delete
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 * Provides the userspace dynamic memory allocator. Links on top of
 * std::alloc::HeapAllocator and grows via the sbrk syscall. Includes
 * global operator new/delete so that userspace programs get working
 * dynamic allocation by linking libstd_heap.a.
 *
 * Auto-initializes on first allocation -- no explicit init required.
 *
 */

#ifndef STD_HEAP_HPP
#define STD_HEAP_HPP

#include <std/alloc.hpp>

namespace std {
namespace heap {

/// Allocate size bytes from the userspace heap. Returns nullptr on failure.
void* alloc(usize size);

/// Free a previously allocated pointer.
void free(void* ptr);

} // namespace heap
} // namespace std

#endif // STD_HEAP_HPP
