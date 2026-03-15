/**
 * paging.cpp
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#include "memory/paging.hpp"
#include "memory/physical.hpp"
#include "memory/virtual.hpp"

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

    // The mmap_addr field is a physical address written by GRUB.
    // Add KERNEL_VBASE to get the virtual address in the direct map.
    u32 mmapAddr = multibootInfo->mmap_addr + KERNEL_VBASE;
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

            // Direct-map: physical address P -> virtual address P + KERNEL_VBASE.
            for (u32 addr = base; addr < end; addr += FRAME_SIZE) {
                mapPage(addr + KERNEL_VBASE, addr, flags);
            }
        }

        mmapAddr += entry->size + sizeof(entry->size);
    }

    // Map VGA memory (physical 0xB8000 - 0xBFFFF) into the direct map.
    for (u32 addr = 0xB8000; addr < 0xC0000; addr += FRAME_SIZE) {
        mapPage(addr + KERNEL_VBASE, addr, flags);
    }

    // Load page directory into CR3. pageDirectory is a virtual address;
    // CR3 needs the physical address.
    u32 pd = (u32)pageDirectory - KERNEL_VBASE;
    asm volatile("mov %0, %%cr3" :: "r"(pd));
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

        // Zero the new page table. allocFrame() returns a physical address;
        // add KERNEL_VBASE to get a dereferenceable virtual pointer.
        u32* pageTable = (u32*)((u32)frame + KERNEL_VBASE);
        for (u32 i = 0; i < 1024; i++) {
            pageTable[i] = 0;
        }

        // Store the physical address in the PDE (hardware requirement).
        pageDirectory[pdIndex] = (u32)frame | PAGE_PRESENT | PAGE_READWRITE;
    }

    // Read the physical address from the PDE, add KERNEL_VBASE to dereference.
    u32* pageTable = (u32*)((pageDirectory[pdIndex] & 0xFFFFF000) + KERNEL_VBASE);
    pageTable[ptIndex] = (physicalAddr & 0xFFFFF000) | (flags & 0xFFF);
}

void PagingManager::unmapPage(u32 virtualAddr) {
    u32 pdIndex = virtualAddr >> 22;
    u32 ptIndex = (virtualAddr >> 12) & 0x3FF;

    if (!(pageDirectory[pdIndex] & PAGE_PRESENT)) {
        return;
    }

    u32* pageTable = (u32*)((pageDirectory[pdIndex] & 0xFFFFF000) + KERNEL_VBASE);
    pageTable[ptIndex] = 0;

    flushTLB(virtualAddr);
}

void PagingManager::flushTLB(u32 virtualAddr) {
    asm volatile("invlpg (%0)" :: "r"(virtualAddr) : "memory");
}
