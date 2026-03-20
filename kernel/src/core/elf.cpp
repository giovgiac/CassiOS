/**
 * elf.cpp
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#include "core/elf.hpp"

#include "memory/paging.hpp"
#include "memory/physical.hpp"
#include "memory/virtual.hpp"

using namespace cassio;
using namespace std;
using namespace cassio::kernel;
using namespace cassio::memory;

ElfLoadResult ElfLoader::load(u32 pdPhysical, const u8* elfData, u32 elfSize) {
    ElfLoadResult result = {0, 0, false};

    if (elfSize < sizeof(Elf32Header)) {
        return result;
    }

    const Elf32Header* header = (const Elf32Header*)elfData;

    // Validate magic.
    if (header->e_ident[EI_MAG0] != ELFMAG0 || header->e_ident[1] != ELFMAG1 ||
        header->e_ident[2] != ELFMAG2 || header->e_ident[3] != ELFMAG3) {
        return result;
    }

    // Validate class (32-bit), type (executable), machine (i386).
    if (header->e_ident[EI_CLASS] != ELFCLASS32) {
        return result;
    }
    if (header->e_type != ET_EXEC) {
        return result;
    }
    if (header->e_machine != EM_386) {
        return result;
    }

    PagingManager& paging = PagingManager::getManager();
    PhysicalMemoryManager& pmm = PhysicalMemoryManager::getManager();

    u32 phoff = header->e_phoff;
    u16 phnum = header->e_phnum;
    u16 phentsize = header->e_phentsize;

    u32 highestEnd = 0;

    for (u16 i = 0; i < phnum; i++) {
        u32 offset = phoff + i * phentsize;
        if (offset + sizeof(Elf32ProgramHeader) > elfSize) {
            return result;
        }

        const Elf32ProgramHeader* ph = (const Elf32ProgramHeader*)(elfData + offset);

        if (ph->p_type != PT_LOAD) {
            continue;
        }

        // Validate p_offset and p_filesz to prevent u32 wrap-around
        // that could read kernel memory into a userspace page.
        if (ph->p_offset > elfSize || ph->p_filesz > elfSize ||
            ph->p_offset + ph->p_filesz < ph->p_offset || ph->p_offset + ph->p_filesz > elfSize) {
            return result;
        }

        // Track the highest virtual address for heap placement.
        u32 segEnd = ph->p_vaddr + ph->p_memsz;
        if (segEnd > highestEnd) {
            highestEnd = segEnd;
        }

        // Allocate physical frames and map into the target address space.
        u32 numPages = (ph->p_memsz + FRAME_SIZE - 1) / FRAME_SIZE;
        for (u32 p = 0; p < numPages; p++) {
            void* frame = pmm.allocFrame();
            if (!frame) {
                return result;
            }

            u32 vaddr = (ph->p_vaddr & 0xFFFFF000) + p * FRAME_SIZE;
            paging.mapUserPage(pdPhysical, vaddr, (u32)frame,
                               PAGE_PRESENT | PAGE_READWRITE | PAGE_USER);

            // Access the frame via the kernel direct map to copy/zero data.
            u8* dest = (u8*)((u32)frame + KERNEL_VBASE);

            u32 pageStart = p * FRAME_SIZE;
            for (u32 b = 0; b < FRAME_SIZE; b++) {
                u32 segOffset = pageStart + b;
                if (segOffset < ph->p_filesz && ph->p_offset + segOffset < elfSize) {
                    dest[b] = elfData[ph->p_offset + segOffset];
                } else {
                    dest[b] = 0;
                }
            }
        }
    }

    result.entryPoint = header->e_entry;
    result.heapStart = (highestEnd + FRAME_SIZE - 1) & ~(FRAME_SIZE - 1);
    result.success = true;
    return result;
}
