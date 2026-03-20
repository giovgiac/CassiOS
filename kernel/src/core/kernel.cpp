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
#include "core/scheduler.hpp"
#include "hardware/pit.hpp"
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
    GlobalDescriptorTable& gdt = *new (gdt_storage) GlobalDescriptorTable;
    InterruptManager& im = InterruptManager::getManager();

    im.load(gdt);

    PhysicalMemoryManager& pmm = PhysicalMemoryManager::getManager();
    pmm.init((MultibootInfo*)multiboot);

    KernelHeap::init();

    PagingManager& paging = PagingManager::getManager();
    paging.init((MultibootInfo*)multiboot);

    PitTimer& pit = PitTimer::getTimer();
    pit.activate();

    // Initialize scheduler and set up kernel task.
    Scheduler& scheduler = Scheduler::getScheduler();
    scheduler.init(gdt);

    ProcessManager& pm = ProcessManager::getManager();
    Process* kernelTask = pm.current();
    kernelTask->state = ProcessState::Running;
    kernelTask->cs = 0x08;
    kernelTask->ds = 0x10;
    kernelTask->pageDirectory = 0;

    // Load userspace processes from multiboot modules.
    MultibootInfo* mb = (MultibootInfo*)multiboot;
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
                continue;
            }

            pm.spawn(pdPhysical, elf.entryPoint, elf.heapStart, userCS, userDS);
        }
    }

    im.activate();

    // Kernel idle loop. Userspace services handle all I/O via IPC.
    while (true) {
        asm volatile("hlt");
    }
}
