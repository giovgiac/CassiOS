/**
 * kernel.cpp
 * 
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#include "core/elf.hpp"
#include "core/kernel.hpp"
#include "core/process.hpp"
#include "core/scheduler.hpp"
#include "drivers/ata.hpp"
#include "drivers/pit.hpp"
#include "memory/heap.hpp"
#include "memory/paging.hpp"
#include "memory/physical.hpp"
#include "memory/virtual.hpp"

using namespace cassio;
using namespace cassio::drivers;
using namespace cassio::hardware;
using namespace cassio::kernel;
using namespace cassio::memory;

void ctors() {
    for (ctor* ct = &start_ctors; ct != &end_ctors; ++ct) {
        (*ct)();
    }
}

void start(void* multiboot, u32 magic) {
    GlobalDescriptorTable gdt;
    InterruptManager& im = InterruptManager::getManager();
    DriverManager& dm = DriverManager::getManager();

    im.load(gdt);

    PhysicalMemoryManager& pmm = PhysicalMemoryManager::getManager();
    pmm.init((MultibootInfo*)multiboot);

    KernelHeap::init();

    PagingManager& paging = PagingManager::getManager();
    paging.init((MultibootInfo*)multiboot);

    // Direct VGA write for pre-service boot message.
    u16* vga_buf = reinterpret_cast<u16*>(KERNEL_VBASE + 0xB8000);
    for (u32 i = 0; i < 80 * 25; ++i) {
        vga_buf[i] = 0x0700 | ' ';
    }
    const char* welcome = "Welcome to the Cassio Operating System!";
    for (u32 i = 0; welcome[i] != '\0'; ++i) {
        vga_buf[i] = 0x0700 | welcome[i];
    }

    PitTimer& pit = PitTimer::getTimer();
    AtaPioDriver& ata = AtaPioDriver::getDriver();

    dm.addDriver(pit);
    dm.addDriver(ata);

    // Initialize scheduler and register kernel task.
    Scheduler& scheduler = Scheduler::getScheduler();
    scheduler.init(gdt);

    ProcessManager& pm = ProcessManager::getManager();
    Process* kernelTask = pm.current();
    kernelTask->state = ProcessState::Running;
    kernelTask->cs = 0x08;
    kernelTask->ds = 0x10;
    kernelTask->pageDirectory = 0;
    scheduler.addProcess(kernelTask);

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

            void* userStackFrame = pmm.allocFrame();
            if (userStackFrame) {
                paging.mapUserPage(pdPhysical, 0xBFFFF000, (u32)userStackFrame,
                                   PAGE_PRESENT | PAGE_READWRITE | PAGE_USER);
            }

            void* kernelStackFrame = pmm.allocFrame();
            if (!kernelStackFrame) {
                continue;
            }

            u32 kernelStackTop = (u32)kernelStackFrame + KERNEL_VBASE + FRAME_SIZE;

            // Build fake interrupt frame on kernel stack for initial iret to ring 3.
            u32* frame = (u32*)kernelStackTop;
            *(--frame) = userDS;            // SS
            *(--frame) = 0xC0000000;        // ESP (top of user stack page)
            *(--frame) = 0x3202;            // EFLAGS (IF=1, IOPL=3)
            *(--frame) = userCS;            // CS
            *(--frame) = elf.entryPoint;    // EIP
            *(--frame) = 0;                 // EAX
            *(--frame) = 0;                 // ECX
            *(--frame) = 0;                 // EDX
            *(--frame) = 0;                 // EBX
            *(--frame) = 0;                 // ESP (ignored by popa)
            *(--frame) = 0;                 // EBP
            *(--frame) = 0;                 // ESI
            *(--frame) = 0;                 // EDI
            *(--frame) = userDS;            // DS
            *(--frame) = userDS;            // ES
            *(--frame) = userDS;            // FS
            *(--frame) = userDS;            // GS

            Process* proc = pm.create(
                elf.entryPoint, (u32)frame, userCS, userDS, pdPhysical);
            if (proc) {
                proc->kernelEsp = kernelStackTop;
                scheduler.addProcess(proc);
            }
        }
    }

    dm.load();

    im.activate();

    // Kernel idle loop. Userspace services handle all I/O via IPC.
    while (true) {
        asm volatile("hlt");
    }
}
