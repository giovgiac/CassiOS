/**
 * kernel.cpp
 * 
 * Copyright (c) 2019 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#include "core/kernel.hpp"

using namespace cassio;
using namespace cassio::drivers;
using namespace cassio::kernel;
using namespace cassio::hardware;

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

    VgaTerminal& vga = VgaTerminal::getTerminal();
    vga.clear();
    vga.print("Welcome to the Cassio Operating System!\n");

    Shell shell;
    KeyboardDriver keyboard(&shell);

    dm.addDriver(keyboard);

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
