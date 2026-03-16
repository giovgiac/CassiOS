/**
 * main.cpp -- keyboard service entry point
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 * Userspace PS/2 keyboard driver. Receives IRQ 1 notifications via IPC,
 * reads scancodes from port 0x60, translates them, and serves characters
 * to clients via MSG_KBD_READ.
 *
 */

#include <types.hpp>
#include <message.hpp>
#include <ipc.hpp>
#include <ns.hpp>
#include <system.hpp>
#include <port.hpp>
#include <keyboard.hpp>

using namespace cassio;
using namespace cassio::hardware;

static Keyboard keyboard;

static void activate() {
    Port<u8> cmd(PortType::KeyboardControllerCommand);
    Port<u8> data(PortType::KeyboardControllerData);

    // Drain any pending scancodes.
    while (cmd.read() & 0x01) {
        data.read();
    }

    // Enable keyboard interface.
    cmd.write(0xAE);

    // Read and modify command byte: enable keyboard IRQs, clear disable bit.
    cmd.write(0x20);
    u8 status = data.read();
    status |= 0x01;   // keyboard_interrupt = 1
    status &= ~0x10;  // disable_keyboard = 0
    cmd.write(0x60);
    data.write(status);

    // Enable keyboard and drain ACK.
    data.write(0xF4);
    data.read();
}

extern "C" void _start() {
    Nameserver::registerName("kbd");
    System::irqRegister(1);
    activate();

    Port<u8> data(PortType::KeyboardControllerData);
    Port<u8> cmd(PortType::KeyboardControllerCommand);

    while (true) {
        Message msg;
        i32 sender = IPC::receive(&msg);

        switch (msg.type) {
        case MessageType::IrqNotify:
            // Drain all pending scancodes from the controller.
            while (cmd.read() & 0x01) {
                keyboard.handleScancode(data.read());
            }
            break;

        case MessageType::KbdRead:
            if (sender > 0) {
                Message reply = {};
                reply.arg1 = static_cast<u8>(keyboard.readBuffer());
                IPC::reply(static_cast<u32>(sender), &reply);
            }
            break;

        default:
            if (sender > 0) {
                Message reply = {};
                IPC::reply(static_cast<u32>(sender), &reply);
            }
            break;
        }
    }
}
