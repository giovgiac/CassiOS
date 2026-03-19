/**
 * heap.hpp -- userspace heap with operator new/delete
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 * Provides the userspace dynamic memory allocator. Links on top of
 * std::alloc::HeapAllocator and grows via a caller-provided callback
 * (typically wrapping the sbrk syscall). Includes global operator
 * new/delete so that userspace programs get working dynamic allocation
 * by linking libstd_heap.a.
 *
 * Note: the GrowFn callback will be replaced by a direct sbrk call
 * once the std::syscall module is migrated (#145).
 *
 */

#ifndef STD_HEAP_HPP
#define STD_HEAP_HPP

#include <std/alloc.hpp>

namespace std {
namespace heap {

class Heap {
public:
    using GrowFn = void*(*)(u32);

    static void init(GrowFn grow, u32 initialSize);
    static void* alloc(usize size);
    static void free(void* ptr);

private:
    static GrowFn growFn;
    static alloc::HeapAllocator* allocator;
};

}
}

#endif // STD_HEAP_HPP
