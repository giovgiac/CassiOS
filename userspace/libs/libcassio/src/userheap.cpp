/**
 * userheap.cpp -- userspace heap allocator
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#include <userheap.hpp>
#include <new.hpp>

using namespace cassio;

UserHeap::GrowFn UserHeap::growFn = nullptr;
HeapAllocator* UserHeap::allocator = nullptr;

alignas(HeapAllocator) static u8 user_heap_storage[sizeof(HeapAllocator)];

void UserHeap::init(GrowFn grow, u32 initialSize) {
    growFn = grow;
    void* base = grow(initialSize);
    if (!base) {
        return;
    }

    // Place the HeapAllocator object in static storage and manage the
    // sbrk'd region (skipping space for the allocator would waste alignment;
    // static storage avoids that).
    allocator = new (user_heap_storage) HeapAllocator(base, initialSize);
}

void* UserHeap::alloc(usize size) {
    if (!allocator) {
        return nullptr;
    }

    void* ptr = allocator->allocate(size);
    if (ptr) {
        return ptr;
    }

    // Allocation failed -- grow the heap and retry.
    u32 needed = size + sizeof(BlockHeader) + 4096;
    needed = (needed + 4095) & ~4095u;
    void* newMem = growFn(needed);
    if (!newMem) {
        return nullptr;
    }

    // sbrk extends contiguously, so extend the allocator's managed region.
    allocator->extend(needed);
    return allocator->allocate(size);
}

void UserHeap::free(void* ptr) {
    if (allocator) {
        allocator->free(ptr);
    }
}
