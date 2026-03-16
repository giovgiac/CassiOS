/**
 * heap.hpp
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#ifndef MEMORY_HEAP_HPP_
#define MEMORY_HEAP_HPP_

#include <types.hpp>

namespace cassio {
namespace memory {

struct BlockHeader {
    u32 size;
    bool free;
    BlockHeader* next;
};

class HeapAllocator {
public:
    HeapAllocator(void* base, u32 size);

    void* allocate(usize size);
    void free(void* ptr);

    HeapAllocator(const HeapAllocator&) = delete;
    HeapAllocator(HeapAllocator&&) = delete;
    HeapAllocator& operator=(const HeapAllocator&) = delete;
    HeapAllocator& operator=(HeapAllocator&&) = delete;

private:
    BlockHeader* head;
};

static constexpr u32 KERNEL_HEAP_FRAMES = 256;
static constexpr u32 KERNEL_HEAP_SIZE = KERNEL_HEAP_FRAMES * 4096;

class KernelHeap final {
public:
    inline static HeapAllocator& getAllocator() {
        return *instance;
    }

    static void init();

    KernelHeap() = delete;

private:
    static HeapAllocator* instance;
};

} // memory
} // cassio

#endif // MEMORY_HEAP_HPP_
