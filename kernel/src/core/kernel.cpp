/**
 * kernel.cpp
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#include "core/kernel.hpp"

#include "core/elf.hpp"
#include "core/process.hpp"
#include "hardware/pit.hpp"
#include "hardware/serial.hpp"
#include "memory/heap.hpp"
#include "memory/paging.hpp"
#include "memory/physical.hpp"
#include "memory/virtual.hpp"

#include <std/alloc.hpp>

using namespace cassio;
using namespace std;
using namespace cassio::hardware;
using namespace cassio::kernel;
using namespace cassio::memory;

// GDT lives at file scope so GDTR and TSS point to stable addresses
// (not the stack). Constructed in start() via placement new.
alignas(GlobalDescriptorTable) static u8 gdt_storage[sizeof(GlobalDescriptorTable)];

void ctors() {
    for (ctor* ct = &start_ctors; ct != &end_ctors; ++ct) {
        (*ct)();
    }
}

void start(void* multiboot, u32 magic) {
    COM1::init();

    GlobalDescriptorTable& gdt = *new (gdt_storage) GlobalDescriptorTable;
    InterruptManager& im = InterruptManager::getManager();

    im.load(gdt);

    PhysicalMemoryManager& pmm = PhysicalMemoryManager::getManager();
    pmm.init((MultibootInfo*)multiboot);

    // Paging first: until it is up only the first 4 MiB are mapped, and
    // its page tables come from just above the kernel. The heap's region
    // after them is reached through the direct map paging sets up.
    PagingManager& paging = PagingManager::getManager();
    paging.init((MultibootInfo*)multiboot);

    KernelHeap::init();

    PitTimer& pit = PitTimer::getTimer();
    pit.activate();

    // The kernel task: the idle loop below, run when nothing else is ready.
    ProcessManager::init();

    // Store framebuffer info from multiboot for the FramebufferInfo syscall.
    MultibootInfo* mb = (MultibootInfo*)multiboot;
    if (mb->flags & MULTIBOOT_FLAG_FRAMEBUFFER) {
        ProcessManager::setFramebuffer(static_cast<u32>(mb->framebuffer_addr),
                                       mb->framebuffer_width, mb->framebuffer_height,
                                       mb->framebuffer_pitch, mb->framebuffer_bpp);
    }

    // Load userspace processes from multiboot modules.
    if ((mb->flags & MULTIBOOT_FLAG_MODS) && mb->mods_count > 0) {
        MultibootModule* mods = (MultibootModule*)(mb->mods_addr + KERNEL_VBASE);

        u32 userCS = gdt.getUserCodeOffset() | 3;
        u32 userDS = gdt.getUserDataOffset() | 3;

        for (u32 i = 0; i < mb->mods_count; i++) {
            const u8* elfData = (const u8*)(mods[i].mod_start + KERNEL_VBASE);
            u32 elfSize = mods[i].mod_end - mods[i].mod_start;

            u32 pdPhysical = paging.createAddressSpace();
            if (!pdPhysical) {
                continue;
            }

            ElfLoadResult elf = ElfLoader::load(pdPhysical, elfData, elfSize);
            if (!elf.success) {
                paging.destroyAddressSpace(pdPhysical);
                continue;
            }

            if (!ProcessManager::spawn(pdPhysical, elf.entryPoint, elf.heapStart, userCS, userDS)) {
                paging.destroyAddressSpace(pdPhysical);
            }
        }
    }

    // Every process has its own copy of its image now.
    pmm.releaseModules(mb);

    im.activate();

    // Kernel idle loop. Userspace services handle all I/O via IPC.
    while (true) {
        asm volatile("hlt");
    }
}
