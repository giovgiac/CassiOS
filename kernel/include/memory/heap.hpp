/**
 * heap.hpp -- kernel heap
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 * The heap is Rust (kernel/src/memory/heap.rs). KernelHeap and operator
 * new/delete (operators.cpp) forward to it until their callers are ported.
 *
 */

#ifndef MEMORY_HEAP_HPP_
#define MEMORY_HEAP_HPP_

#include <std/types.hpp>

extern "C" void heap_init();
extern "C" void* kernel_alloc(std::usize size);
extern "C" void kernel_free(void* ptr);

namespace cassio {
namespace memory {

class KernelHeap final {
public:
    /** Reserves the heap region; allocations fail until this runs. */
    static void init() { heap_init(); }

    KernelHeap() = delete;
};

} // namespace memory
} // namespace cassio

#endif // MEMORY_HEAP_HPP_
