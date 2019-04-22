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

void clear_screen() {
    static u16* tty = reinterpret_cast<u16*>(0xb8000);

    for (u8 y = 0; y < TERMINAL_HEIGHT; ++y) {
        for (u8 x = 0; x < TERMINAL_WIDTH; ++x) {
            // Copy High-Bits and Merge with New Low-Bits
            tty[TERMINAL_WIDTH * y + x] = (tty[TERMINAL_WIDTH * y + x] & 0xFF00) | ' ';
        }
    }
}

void outputs(const char* str) {
    static u16* tty = reinterpret_cast<u16*>(0xb8000);
    static u8 x = 0, y = 0;

    for (u32 i = 0; str[i] != '\0' ; ++i) {
        switch(str[i]) {
        case '\n':
            y += 1;
            x = 0;

            break;
        default:
            // Copy High-Bits and Merge with New Low-Bits
            tty[TERMINAL_WIDTH * y + x] = (tty[TERMINAL_WIDTH * y + x] & 0xFF00) | str[i];
            x += 1;

            break;
        }

        if (x >= TERMINAL_WIDTH) {
            y += 1;
            x = 0;
        }

        if (y >= TERMINAL_HEIGHT) {
            clear_screen();
            x = y = 0;
        }
    }
}

void ctors() {
    for (ctor* ct = &start_ctors; ct != &end_ctors; ++ct) {
        (*ct)();
    }
}

void start(void* multiboot, u32 magic) {
    outputs("Welcome to CassiOS!\n");

    GlobalDescriptorTable gdt;
    InterruptManager& im = InterruptManager::getManager();
    im.load(gdt);
    
    KeyboardDriver keyboard;

    im.activate();

    while (1);

    im.deactivate();
    im.unload();
}
