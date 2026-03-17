/**
 * userheap.hpp -- userspace heap allocator
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#ifndef COMMON_USERHEAP_HPP_
#define COMMON_USERHEAP_HPP_

#include <types.hpp>
#include <heap.hpp>

namespace cassio {

/**
 * @brief Userspace heap built on top of sbrk.
 *
 * Manages dynamic memory allocation for userspace services. Uses a
 * callback to grow the heap region (typically wrapping the sbrk syscall).
 *
 */
class UserHeap {
public:
    using GrowFn = void*(*)(u32);

    static void init(GrowFn grow, u32 initialSize);
    static void* alloc(usize size);
    static void free(void* ptr);

private:
    static GrowFn growFn;
    static HeapAllocator* allocator;
};

} // cassio

#endif // COMMON_USERHEAP_HPP_
