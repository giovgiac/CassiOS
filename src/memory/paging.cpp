/**
 * paging.cpp
 *
 * Copyright (c) 2019 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#include "memory/paging.hpp"
#include "memory/physical.hpp"

using namespace cassio;
using namespace cassio::memory;

PagingManager PagingManager::instance;

PagingManager::PagingManager() {
    for (u32 i = 0; i < 1024; i++) {
        pageDirectory[i] = 0;
    }
}

void PagingManager::init(MultibootInfo* multibootInfo) {
    if (!(multibootInfo->flags & MULTIBOOT_FLAG_MMAP)) {
        return;
    }

    u16 flags = PAGE_PRESENT | PAGE_READWRITE;

    // Identity-map all available memory regions from the multiboot memory map.
    u32 mmapAddr = multibootInfo->mmap_addr;
    u32 mmapEnd = mmapAddr + multibootInfo->mmap_length;

    while (mmapAddr < mmapEnd) {
        MultibootMmapEntry* entry = (MultibootMmapEntry*)mmapAddr;

        if (entry->type == MULTIBOOT_MMAP_AVAILABLE) {
            u32 base = (u32)entry->base_addr;
            u32 end;
            if (entry->base_addr + entry->length > 0xFFFFFFFF) {
                end = 0xFFFFF000;
            } else {
                end = (u32)(entry->base_addr + entry->length);
            }

            // Align base down and end up to page boundaries.
            base &= 0xFFFFF000;
            end = (end + FRAME_SIZE - 1) & 0xFFFFF000;

            for (u32 addr = base; addr < end; addr += FRAME_SIZE) {
                mapPage(addr, addr, flags);
            }
        }

        mmapAddr += entry->size + sizeof(entry->size);
    }

    // Map VGA memory (0xB8000 - 0xBFFFF).
    for (u32 addr = 0xB8000; addr < 0xC0000; addr += FRAME_SIZE) {
        mapPage(addr, addr, flags);
    }

    // Load page directory into CR3 and enable paging in CR0.
    u32 pd = (u32)pageDirectory;
    asm volatile("mov %0, %%cr3" :: "r"(pd));

    u32 cr0;
    asm volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= 0x80000000;
    asm volatile("mov %0, %%cr0" :: "r"(cr0));
}

void PagingManager::mapPage(u32 virtualAddr, u32 physicalAddr, u16 flags) {
    u32 pdIndex = virtualAddr >> 22;
    u32 ptIndex = (virtualAddr >> 12) & 0x3FF;

    // Allocate a page table if this directory entry is empty.
    if (!(pageDirectory[pdIndex] & PAGE_PRESENT)) {
        PhysicalMemoryManager& pmm = PhysicalMemoryManager::getManager();
        void* frame = pmm.allocFrame();
        if (!frame) {
            return;
        }

        // Zero the new page table.
        u32* pageTable = (u32*)frame;
        for (u32 i = 0; i < 1024; i++) {
            pageTable[i] = 0;
        }

        pageDirectory[pdIndex] = (u32)frame | PAGE_PRESENT | PAGE_READWRITE;
    }

    u32* pageTable = (u32*)(pageDirectory[pdIndex] & 0xFFFFF000);
    pageTable[ptIndex] = (physicalAddr & 0xFFFFF000) | (flags & 0xFFF);
}

void PagingManager::unmapPage(u32 virtualAddr) {
    u32 pdIndex = virtualAddr >> 22;
    u32 ptIndex = (virtualAddr >> 12) & 0x3FF;

    if (!(pageDirectory[pdIndex] & PAGE_PRESENT)) {
        return;
    }

    u32* pageTable = (u32*)(pageDirectory[pdIndex] & 0xFFFFF000);
    pageTable[ptIndex] = 0;

    flushTLB(virtualAddr);
}

void PagingManager::flushTLB(u32 virtualAddr) {
    asm volatile("invlpg (%0)" :: "r"(virtualAddr) : "memory");
}
