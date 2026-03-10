/**
 * heap.cpp
 *
 * Copyright (c) 2019 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#include "memory/heap.hpp"
#include "memory/physical.hpp"

using namespace cassio;
using namespace cassio::memory;

static constexpr u32 MIN_BLOCK_SIZE = sizeof(BlockHeader) + 4;

HeapAllocator HeapAllocator::instance;

HeapAllocator::HeapAllocator()
    : head(nullptr) {
}

void HeapAllocator::init() {
    PhysicalMemoryManager& pmm = PhysicalMemoryManager::getManager();

    // Allocate contiguous frames for the heap.
    void* base = pmm.allocFrame();
    if (!base) {
        return;
    }

    for (u32 i = 1; i < HEAP_FRAMES; i++) {
        pmm.allocFrame();
    }

    head = (BlockHeader*)base;
    head->size = HEAP_SIZE - sizeof(BlockHeader);
    head->free = true;
    head->next = nullptr;
}

void* HeapAllocator::allocate(usize size) {
    if (size == 0) {
        return nullptr;
    }

    // Align size to 4 bytes.
    size = (size + 3) & ~3u;

    BlockHeader* current = head;
    while (current) {
        if (current->free && current->size >= size) {
            // Split if remainder is large enough for another block.
            u32 remainder = current->size - size;
            if (remainder >= MIN_BLOCK_SIZE) {
                BlockHeader* newBlock = (BlockHeader*)((u8*)current + sizeof(BlockHeader) + size);
                newBlock->size = remainder - sizeof(BlockHeader);
                newBlock->free = true;
                newBlock->next = current->next;

                current->size = size;
                current->next = newBlock;
            }

            current->free = false;
            return (void*)((u8*)current + sizeof(BlockHeader));
        }
        current = current->next;
    }

    return nullptr;
}

void HeapAllocator::free(void* ptr) {
    if (!ptr) {
        return;
    }

    BlockHeader* block = (BlockHeader*)((u8*)ptr - sizeof(BlockHeader));
    block->free = true;

    // Coalesce adjacent free blocks.
    BlockHeader* current = head;
    while (current) {
        if (current->free && current->next && current->next->free) {
            current->size += sizeof(BlockHeader) + current->next->size;
            current->next = current->next->next;
            continue;
        }
        current = current->next;
    }
}
