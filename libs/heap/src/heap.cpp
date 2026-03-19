/**
 * heap.cpp -- userspace heap and operator new/delete
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#include <std/heap.hpp>
#include <std/os.hpp>

using namespace std;

alloc::HeapAllocator* heap::Heap::allocator = nullptr;

alignas(alloc::HeapAllocator) static u8 heap_storage[sizeof(alloc::HeapAllocator)];

static constexpr u32 INITIAL_HEAP_SIZE = 4096;

void* heap::Heap::alloc(usize size) {
    if (!allocator) {
        void* base = os::sbrk(INITIAL_HEAP_SIZE);
        if (!base) {
            return nullptr;
        }
        allocator = new (heap_storage) alloc::HeapAllocator(base, INITIAL_HEAP_SIZE);
    }

    void* ptr = allocator->allocate(size);
    if (ptr) {
        return ptr;
    }

    // Allocation failed -- grow the heap and retry.
    u32 needed = size + sizeof(alloc::BlockHeader) + 4096;
    needed = (needed + 4095) & ~4095u;
    void* newMem = os::sbrk(needed);
    if (!newMem) {
        return nullptr;
    }

    allocator->extend(needed);
    return allocator->allocate(size);
}

void heap::Heap::free(void* ptr) {
    if (allocator) {
        allocator->free(ptr);
    }
}

// Global operator new/delete for userspace programs.

void* operator new(usize size) {
    return heap::Heap::alloc(size);
}

void* operator new[](usize size) {
    return heap::Heap::alloc(size);
}

void operator delete(void* ptr) {
    heap::Heap::free(ptr);
}

void operator delete[](void* ptr) {
    heap::Heap::free(ptr);
}

void operator delete(void* ptr, usize) {
    heap::Heap::free(ptr);
}

void operator delete[](void* ptr, usize) {
    heap::Heap::free(ptr);
}
