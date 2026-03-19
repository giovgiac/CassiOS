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

#include <std/types.hpp>
#include <std/msg.hpp>
#include <ipc.hpp>
#include <ns.hpp>
#include <system.hpp>
#include <std/io.hpp>
#include <keyboard.hpp>

using namespace cassio;
using namespace std;
using namespace std::io;

static Keyboard keyboard;

static void activate() {
    Port<u8> cmd(PortType::KbdCommand);
    Port<u8> data(PortType::KbdData);

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

    Port<u8> data(PortType::KbdData);
    Port<u8> cmd(PortType::KbdCommand);

    // Pending reader: if a KbdRead arrives when the buffer is empty,
    // we hold the sender and reply only when a character becomes available.
    u32 pending_reader = 0;

    while (true) {
        msg::Message msg;
        i32 sender = IPC::receive(&msg);

        switch (msg.type) {
        case msg::MessageType::IrqNotify:
            // Drain all pending scancodes from the controller.
            while (cmd.read() & 0x01) {
                keyboard.handleScancode(data.read());
            }

            // Wake a blocked reader if characters are now available.
            if (pending_reader != 0 && keyboard.bufferCount() > 0) {
                msg::Message reply = {};
                reply.arg1 = static_cast<u8>(keyboard.readBuffer());
                IPC::reply(pending_reader, &reply);
                pending_reader = 0;
            }
            break;

        case msg::MessageType::KbdRead:
            if (sender > 0) {
                if (keyboard.bufferCount() > 0) {
                    msg::Message reply = {};
                    reply.arg1 = static_cast<u8>(keyboard.readBuffer());
                    IPC::reply(static_cast<u32>(sender), &reply);
                } else {
                    // Buffer empty -- hold the request until a character arrives.
                    pending_reader = static_cast<u32>(sender);
                }
            }
            break;

        default:
            if (sender > 0) {
                msg::Message reply = {};
                IPC::reply(static_cast<u32>(sender), &reply);
            }
            break;
        }
    }
}
