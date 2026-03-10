/**
 * physical.cpp
 *
 * Copyright (c) 2019 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#include "memory/physical.hpp"

using namespace cassio;
using namespace cassio::memory;

extern "C" u32 _kernel_start;
extern "C" u32 _kernel_end;

PhysicalMemoryManager PhysicalMemoryManager::instance;

PhysicalMemoryManager::PhysicalMemoryManager()
    : totalFrames(0) {
    for (u32 i = 0; i < BITMAP_SIZE; i++) {
        bitmap[i] = 0xFF;
    }
}

void PhysicalMemoryManager::init(MultibootInfo* multibootInfo) {
    if (!(multibootInfo->flags & MULTIBOOT_FLAG_MMAP)) {
        return;
    }

    u32 mmapAddr = multibootInfo->mmap_addr;
    u32 mmapEnd = mmapAddr + multibootInfo->mmap_length;

    while (mmapAddr < mmapEnd) {
        MultibootMmapEntry* entry = (MultibootMmapEntry*)mmapAddr;

        if (entry->type == MULTIBOOT_MMAP_AVAILABLE) {
            u32 base = (u32)entry->base_addr;
            u32 length = (u32)entry->length;

            if (entry->base_addr + entry->length > 0xFFFFFFFF) {
                length = 0xFFFFFFFF - base;
            }

            markRegionFree(base, length);
        }

        mmapAddr += entry->size + sizeof(entry->size);
    }

    // Count total usable frames (all available RAM reported by multiboot).
    totalFrames = 0;
    for (u32 i = 0; i < BITMAP_SIZE; i++) {
        for (u8 bit = 0; bit < 8; bit++) {
            if (!(bitmap[i] & (1 << bit))) {
                totalFrames++;
            }
        }
    }

    // Re-mark kernel, bitmap, and null page as used.
    u32 kernelStart = (u32)&_kernel_start;
    u32 kernelEnd = (u32)&_kernel_end;
    markRegionUsed(kernelStart, kernelEnd - kernelStart);

    u32 bitmapAddr = (u32)bitmap;
    markRegionUsed(bitmapAddr, BITMAP_SIZE);

    markRegionUsed(0, FRAME_SIZE);
}

void PhysicalMemoryManager::markRegionUsed(u32 base, u32 length) {
    u32 startFrame = base / FRAME_SIZE;
    u32 endFrame = (base + length + FRAME_SIZE - 1) / FRAME_SIZE;

    for (u32 frame = startFrame; frame < endFrame; frame++) {
        u32 byteIndex = frame / 8;
        u8 bitIndex = frame % 8;
        if (byteIndex < BITMAP_SIZE) {
            bitmap[byteIndex] |= (1 << bitIndex);
        }
    }
}

void PhysicalMemoryManager::markRegionFree(u32 base, u32 length) {
    u32 startFrame = (base + FRAME_SIZE - 1) / FRAME_SIZE;
    u32 endFrame = (base + length) / FRAME_SIZE;

    for (u32 frame = startFrame; frame < endFrame; frame++) {
        u32 byteIndex = frame / 8;
        u8 bitIndex = frame % 8;
        if (byteIndex < BITMAP_SIZE) {
            bitmap[byteIndex] &= ~(1 << bitIndex);
        }
    }
}

void* PhysicalMemoryManager::allocFrame() {
    for (u32 i = 0; i < BITMAP_SIZE; i++) {
        if (bitmap[i] == 0xFF) {
            continue;
        }

        for (u8 bit = 0; bit < 8; bit++) {
            if (!(bitmap[i] & (1 << bit))) {
                bitmap[i] |= (1 << bit);
                return (void*)((i * 8 + bit) * FRAME_SIZE);
            }
        }
    }

    return nullptr;
}

void PhysicalMemoryManager::freeFrame(void* address) {
    u32 frame = (u32)address / FRAME_SIZE;
    u32 byteIndex = frame / 8;
    u8 bitIndex = frame % 8;

    if (byteIndex < BITMAP_SIZE) {
        bitmap[byteIndex] &= ~(1 << bitIndex);
    }
}

bool PhysicalMemoryManager::isFrameUsed(void* address) const {
    u32 frame = (u32)address / FRAME_SIZE;
    u32 byteIndex = frame / 8;
    u8 bitIndex = frame % 8;

    if (byteIndex >= BITMAP_SIZE) {
        return true;
    }

    return bitmap[byteIndex] & (1 << bitIndex);
}

u32 PhysicalMemoryManager::getTotalFrames() const {
    return totalFrames;
}

u32 PhysicalMemoryManager::getFreeFrames() const {
    u32 free = 0;
    for (u32 i = 0; i < BITMAP_SIZE; i++) {
        for (u8 bit = 0; bit < 8; bit++) {
            if (!(bitmap[i] & (1 << bit))) {
                free++;
            }
        }
    }
    return free;
}

u32 PhysicalMemoryManager::getUsedFrames() const {
    return totalFrames - getFreeFrames();
}
