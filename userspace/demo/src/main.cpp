/**
 * main.cpp -- keyboard echo demo
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 * Minimal process that reads keystrokes from the kbd service
 * and echoes them to the VGA terminal service.
 *
 */

#include <types.hpp>
#include <message.hpp>
#include <ipc.hpp>
#include <ns.hpp>
#include <vga.hpp>

using namespace cassio;

extern "C" void _start() {
    // Wait for kbd and vga services to register.
    u32 kbd_pid = 0;
    while (kbd_pid == 0) {
        kbd_pid = Nameserver::lookup("kbd");
    }

    u32 vga_pid = 0;
    while (vga_pid == 0) {
        vga_pid = Nameserver::lookup("vga");
    }

    while (true) {
        // Blocking read: returns one character.
        Message msg = {};
        msg.type = MessageType::KbdRead;
        IPC::send(kbd_pid, &msg);

        char ch = static_cast<char>(msg.arg1);
        if (ch != '\0') {
            Vga::putchar(vga_pid, ch);
        }
    }
}
