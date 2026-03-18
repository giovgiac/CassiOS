/**
 * heap.hpp -- generic heap allocator
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#ifndef COMMON_HEAP_HPP_
#define COMMON_HEAP_HPP_

#include <std/types.hpp>

namespace cassio {

struct BlockHeader {
    std::u32 size;
    bool free;
    BlockHeader* next;
};

class HeapAllocator {
public:
    HeapAllocator(void* base, std::u32 size);

    void* allocate(std::usize size);
    void free(void* ptr);

    /**
     * @brief Extends the managed region by additionalSize bytes.
     *
     * The caller must ensure that the new memory is contiguous with the
     * existing heap region (e.g. via sbrk).
     *
     */
    void extend(std::u32 additionalSize);

    HeapAllocator(const HeapAllocator&) = delete;
    HeapAllocator(HeapAllocator&&) = delete;
    HeapAllocator& operator=(const HeapAllocator&) = delete;
    HeapAllocator& operator=(HeapAllocator&&) = delete;

private:
    BlockHeader* head;
    std::u8* regionStart;
    std::u8* regionEnd;
};

} // cassio

#endif // COMMON_HEAP_HPP_
