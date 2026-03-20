/**
 * main.cpp -- mouse service entry point
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 * Userspace PS/2 mouse driver. Receives IRQ 12 notifications via IPC,
 * reads mouse data from port 0x60, and serves mouse state to clients
 * via MSG_MOUSE_READ.
 *
 */

#include <std/io.hpp>
#include <std/ipc.hpp>
#include <std/ns.hpp>
#include <std/os.hpp>
#include <std/types.hpp>

#include <mouse.hpp>

using namespace cassio;
using namespace std;
using namespace std::io;

static Mouse mouse;

static void activate() {
    Port<u8> cmd(PortType::KbdCommand);
    Port<u8> data(PortType::KbdData);

    // Enable mouse on PS/2 controller.
    cmd.write(0xA8);

    // Read and modify command byte: enable mouse IRQ (bit 1).
    cmd.write(0x20);
    u8 status = data.read() | 0x02;
    cmd.write(0x60);
    data.write(status);

    // Send 0xF4 (enable reporting) to mouse device.
    cmd.write(0xD4);
    data.write(0xF4);
    data.read(); // Drain ACK
}

extern "C" void _start() {
    ns::registerName("mouse");
    os::irqRegister(12);
    mouse.init();
    activate();

    Port<u8> cmd(PortType::KbdCommand);
    Port<u8> data(PortType::KbdData);

    while (true) {
        ipc::Message msg;
        i32 sender = ipc::receive(&msg);

        switch (msg.type) {
        case ipc::MessageType::IrqNotify:
            // Read all pending mouse bytes.
            while (cmd.read() & 0x21) {
                mouse.handleByte(data.read());
            }
            break;

        case ipc::MessageType::MouseRead:
            if (sender > 0) {
                u8 btns;
                i32 mx, my;
                mouse.readState(btns, mx, my);

                ipc::Message reply = {};
                reply.arg1 = btns;
                reply.arg2 = static_cast<u32>(mx);
                reply.arg3 = static_cast<u32>(my);
                ipc::reply(static_cast<u32>(sender), &reply);
            }
            break;

        default:
            if (sender > 0) {
                ipc::Message reply = {};
                ipc::reply(static_cast<u32>(sender), &reply);
            }
            break;
        }
    }
}
