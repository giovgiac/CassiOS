/**
 * main.cpp -- keyboard echo demo
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 * Minimal process that reads keystrokes from the kbd service
 * and echoes them to the VGA terminal via System::write.
 *
 */

#include <types.hpp>
#include <message.hpp>
#include <ipc.hpp>
#include <ns.hpp>
#include <system.hpp>

using namespace cassio;

extern "C" void _start() {
    // Wait for kbd service to register.
    u32 kbd_pid = 0;
    while (kbd_pid == 0) {
        kbd_pid = Nameserver::lookup("kbd");
    }

    while (true) {
        Message msg = {};
        msg.type = MessageType::KbdRead;
        IPC::send(kbd_pid, &msg);

        char ch = static_cast<char>(msg.arg1);
        if (ch != '\0') {
            if (ch == '\n') {
                System::write(1, "\n", 1);
            } else {
                System::write(1, &ch, 1);
            }
        }
    }
}
