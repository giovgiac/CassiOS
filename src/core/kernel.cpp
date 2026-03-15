/**
 * kernel.cpp
 * 
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#include "core/kernel.hpp"
#include "core/process.hpp"
#include "core/scheduler.hpp"
#include "core/shell.hpp"
#include "core/syscall.hpp"
#include "drivers/ata.hpp"
#include "drivers/keyboard.hpp"
#include "drivers/pit.hpp"
#include "memory/heap.hpp"
#include "memory/paging.hpp"
#include "memory/physical.hpp"

using namespace cassio;
using namespace cassio::drivers;
using namespace cassio::kernel;
using namespace cassio::hardware;
using namespace cassio::memory;

static u8 taskAStack[4096] __attribute__((aligned(16)));
static u8 taskBStack[4096] __attribute__((aligned(16)));

static void taskA() {
    VgaTerminal& vga = VgaTerminal::getTerminal();
    PitTimer& pit = PitTimer::getTimer();
    while (true) {
        vga.print("[A]");
        pit.sleep(1000);
    }
}

static void taskB() {
    VgaTerminal& vga = VgaTerminal::getTerminal();
    PitTimer& pit = PitTimer::getTimer();
    while (true) {
        vga.print("[B]");
        pit.sleep(1000);
    }
}

static u32 buildInterruptFrame(u8* stackTop, u32 eip) {
    u32* sp = (u32*)stackTop;
    *(--sp) = 0x202;    // eflags (IF=1)
    *(--sp) = 0x08;     // cs (kernel code)
    *(--sp) = eip;      // eip
    // pusha order: eax, ecx, edx, ebx, esp, ebp, esi, edi
    for (i32 i = 0; i < 8; i++) *(--sp) = 0;
    // segment registers: ds, es, fs, gs
    *(--sp) = 0x10;     // ds
    *(--sp) = 0x10;     // es
    *(--sp) = 0x10;     // fs
    *(--sp) = 0x10;     // gs
    return (u32)sp;
}

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

    SyscallHandler& sh = SyscallHandler::getSyscallHandler();
    sh.load();

    PhysicalMemoryManager& pmm = PhysicalMemoryManager::getManager();
    pmm.init((MultibootInfo*)multiboot);

    KernelHeap::init();

    PagingManager& paging = PagingManager::getManager();
    paging.init((MultibootInfo*)multiboot);

    VgaTerminal& vga = VgaTerminal::getTerminal();
    vga.clear();
    vga.print("Welcome to the Cassio Operating System!\n");

    Shell shell;

    KeyboardDriver& keyboard = KeyboardDriver::getDriver();
    keyboard.setHandler(&shell);

    PitTimer& pit = PitTimer::getTimer();
    AtaPioDriver& ata = AtaPioDriver::getDriver();

    dm.addDriver(pit);
    dm.addDriver(keyboard);
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

    // Create two demo kernel tasks for manual scheduling verification.
    u32 espA = buildInterruptFrame(taskAStack + sizeof(taskAStack), (u32)taskA);
    Process* pA = pm.create((u32)taskA, espA, 0x08, 0x10, 0);
    scheduler.addProcess(pA);

    u32 espB = buildInterruptFrame(taskBStack + sizeof(taskBStack), (u32)taskB);
    Process* pB = pm.create((u32)taskB, espB, 0x08, 0x10, 0);
    scheduler.addProcess(pB);

    dm.load();

    im.activate();

    shell.run();

    im.deactivate();

    dm.unload();
    im.unload();

    // Halt the CPU. Interrupts are already disabled, so this stops execution.
    while (true) {
        asm volatile("hlt");
    }
}
