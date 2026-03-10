/**
 * heap.hpp
 *
 * Copyright (c) 2019 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#ifndef MEMORY_HEAP_HPP_
#define MEMORY_HEAP_HPP_

#include <common/types.hpp>

namespace cassio {
namespace memory {

static constexpr u32 HEAP_FRAMES = 256;
static constexpr u32 HEAP_SIZE = HEAP_FRAMES * 4096;

struct BlockHeader {
    u32 size;
    bool free;
    BlockHeader* next;
};

class HeapAllocator {
public:
    inline static HeapAllocator& getAllocator() {
        return instance;
    }

    void init();

    void* allocate(usize size);
    void free(void* ptr);

    HeapAllocator(const HeapAllocator&) = delete;
    HeapAllocator(HeapAllocator&&) = delete;
    HeapAllocator& operator=(const HeapAllocator&) = delete;
    HeapAllocator& operator=(HeapAllocator&&) = delete;

private:
    HeapAllocator();

    static HeapAllocator instance;

    BlockHeader* head;
};

} // memory
} // cassio

#endif // MEMORY_HEAP_HPP_
