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
        if (key != KeyCode::Enter)
            std::cout << static_cast<char>(key);
        else
            std::cout << '\n';
    }
};

class TestMouseEventHandler : public MouseEventHandler {
private:
    i8 x = 40, y = 12;
    u16* tty = reinterpret_cast<u16*>(0xB8000);

public:
    virtual void OnActivate() override {
        // Show cursor initial position.
        tty[80 * y + x] = ((tty[80 * y + x] & 0xF000) >> 4) |
                        ((tty[80 * y + x] & 0x0F00) << 4) |
                        ((tty[80 * y + x] & 0x00FF));
    }

    virtual void OnMouseMove(i8 dx, i8 dy) override {
        // Unshow cursor at old position by inverting color at position.
        tty[80 * y + x] = ((tty[80 * y + x] & 0xF000) >> 4) |
                        ((tty[80 * y + x] & 0x0F00) << 4) |
                        ((tty[80 * y + x] & 0x00FF));

        // Update x coordinate of mouse.
        x += dx;
        if (x < 00) x = 00;
        if (x > 79) x = 79;

        // Update y coordinate of mouse.
        y += dy;
        if (y < 00) y = 00;
        if (y > 24) y = 24;

        // Show cursor on terminal by inverting colors at position.
        tty[80 * y + x] = ((tty[80 * y + x] & 0xF000) >> 4) |
                        ((tty[80 * y + x] & 0x0F00) << 4) |
                        ((tty[80 * y + x] & 0x00FF));
    }
};

void ctors() {
    for (ctor* ct = &start_ctors; ct != &end_ctors; ++ct) {
        (*ct)();
    }
}

void start(void* multiboot, u32 magic) {
    std::cout << "WELCOME TO CASSIO OPERATING SYSTEM!\n";

    GlobalDescriptorTable gdt;
    InterruptManager& im = InterruptManager::getManager();
    DriverManager& dm = DriverManager::getManager();

    im.load(gdt);

    std::cout << "STARTING UP DRIVERS...\n";
    
    TestKeyboardEventHandler keyboard_handler;
    TestMouseEventHandler mouse_handler;

    KeyboardDriver keyboard (&keyboard_handler);
    MouseDriver mouse (&mouse_handler);

    dm.addDriver(keyboard);
    dm.addDriver(mouse);

    dm.load();

    im.activate();

    std::cout << "FINISHED STARTING UP DRIVERS...\n";

    while (1);

    im.deactivate();

    dm.unload();
    im.unload();
}
