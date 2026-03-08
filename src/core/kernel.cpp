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

class TestKeyboardEventHandler : public KeyboardEventHandler {
public:
    virtual void OnKeyDown(KeyCode key) override {
        VgaTerminal& vga = VgaTerminal::getTerminal();
        u8 ch = static_cast<u8>(key);

        if (key == KeyCode::Enter)
            vga.putchar('\n');
        else if (key == KeyCode::Backspace)
            vga.putchar('\b');
        else if (key == KeyCode::Tab)
            vga.putchar('\t');
        else if (key == KeyCode::Delete)
            vga.putchar(0x7F);
        else if (ch >= 0x20 && ch <= 0x7E)
            vga.putchar(static_cast<char>(key));
    }
};

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
    vga.print("Starting up drivers...\n");

    TestKeyboardEventHandler keyboard_handler;
    KeyboardDriver keyboard (&keyboard_handler);

    dm.addDriver(keyboard);

    dm.load();

    im.activate();

    vga.print("Finished starting up drivers.\n");

    while (1);

    im.deactivate();

    dm.unload();
    im.unload();
}
