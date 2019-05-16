/**
 * kernel.cpp
 * 
 * Copyright (c) 2019 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#include "core/kernel.hpp"

using namespace cassio::drivers;
using namespace cassio::kernel;
using namespace cassio::hardware;

void ctors() {
    for (ctor* ct = &start_ctors; ct != &end_ctors; ++ct) {
        (*ct)();
    }
}

void start(void* multiboot, u32 magic) {
    std::cout << "Welcome to CassiOS!\n";

    GlobalDescriptorTable gdt;
    InterruptManager& im = InterruptManager::getManager();
    DriverManager& dm = DriverManager::getManager();

    im.load(gdt);

    std::cout << "Starting Up Drivers...\n";
    
    KeyboardEventHandler handler;
    KeyboardDriver keyboard (&handler);
    MouseDriver mouse;

    dm.addDriver(keyboard);
    dm.addDriver(mouse);

    dm.load();

    im.activate();

    std::cout << "Finished Starting Up Drivers...\n";

    while (1);

    im.deactivate();

    dm.unload();
    im.unload();
}
