/**
 * alloc.hpp -- generic allocators and placement new
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#ifndef STD_ALLOC_HPP
#define STD_ALLOC_HPP

#include <std/types.hpp>

// Placement new operator.
inline void* operator new(std::usize, void* ptr) {
    return ptr;
}

namespace std {
namespace alloc {

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

    /**
     * Extends the managed region by additionalSize bytes.
     *
     * The caller must ensure that the new memory is contiguous with the
     * existing heap region (e.g. via sbrk).
     */
    void extend(u32 additionalSize);

    HeapAllocator(const HeapAllocator&) = delete;
    HeapAllocator(HeapAllocator&&) = delete;
    HeapAllocator& operator=(const HeapAllocator&) = delete;
    HeapAllocator& operator=(HeapAllocator&&) = delete;

private:
    BlockHeader* head;
    u8* regionStart;
    u8* regionEnd;
};

}
}

#endif // STD_ALLOC_HPP
