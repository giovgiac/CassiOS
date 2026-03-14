/**
 * kernel.cpp
 * 
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#include "core/kernel.hpp"
#include "core/shell.hpp"
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
