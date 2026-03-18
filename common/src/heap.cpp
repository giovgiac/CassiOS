/**
 * heap.cpp -- generic heap allocator
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#include <heap.hpp>

using namespace cassio;

static constexpr u32 MIN_BLOCK_SIZE = sizeof(BlockHeader) + 4;

HeapAllocator::HeapAllocator(void* base, u32 size)
    : head(nullptr), regionStart(nullptr), regionEnd(nullptr) {
    if (!base || size <= sizeof(BlockHeader)) {
        return;
    }

    head = (BlockHeader*)base;
    head->size = size - sizeof(BlockHeader);
    head->free = true;
    head->next = nullptr;

    regionStart = (u8*)base;
    regionEnd = (u8*)base + size;
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

    u8* raw = (u8*)ptr;

    // Bounds check: reject pointers outside the managed region.
    if (raw < regionStart + sizeof(BlockHeader) || raw >= regionEnd) {
        return;
    }

    BlockHeader* block = (BlockHeader*)(raw - sizeof(BlockHeader));

    // Double-free protection: ignore if already free.
    if (block->free) {
        return;
    }

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

void HeapAllocator::extend(u32 additionalSize) {
    if (additionalSize <= sizeof(BlockHeader) || !head) {
        return;
    }

    // Find the last block.
    BlockHeader* last = head;
    while (last->next) {
        last = last->next;
    }

    if (last->free) {
        // Extend the existing last free block.
        last->size += additionalSize;
    } else {
        // Create a new free block after the last used block.
        BlockHeader* newBlock = (BlockHeader*)((u8*)last + sizeof(BlockHeader) + last->size);
        newBlock->size = additionalSize - sizeof(BlockHeader);
        newBlock->free = true;
        newBlock->next = nullptr;
        last->next = newBlock;
    }

    regionEnd += additionalSize;
}
