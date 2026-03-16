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
        // Blocking batch read: returns all buffered chars packed in arg1-arg5.
        Message msg = {};
        msg.type = MessageType::KbdRead;
        IPC::send(kbd_pid, &msg);

        // Fire-and-forget: send batch to vga without blocking.
        msg.type = MessageType::VgaWrite;
        IPC::notify(vga_pid, &msg);
    }
}
