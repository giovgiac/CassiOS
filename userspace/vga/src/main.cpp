/**
 * main.cpp -- VGA terminal service entry point
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 * Userspace VGA text-mode terminal service. Registers as "vga" with
 * the nameserver and handles display requests via IPC. Maps VGA
 * memory (physical 0xB8000) into its own address space on startup.
 *
 */

#include <types.hpp>
#include <message.hpp>
#include <ipc.hpp>
#include <ns.hpp>
#include <system.hpp>
#include <terminal.hpp>

using namespace cassio;

static constexpr u32 VGA_PHYSICAL = 0xB8000;

extern "C" void _start() {
    Nameserver::registerName("vga");

    // Map VGA buffer into our address space.
    System::mapDevice(VGA_PHYSICAL, VGA_PHYSICAL, 1);

    VgaTerminal terminal(reinterpret_cast<u16*>(VGA_PHYSICAL));
    terminal.clear();

    const char* welcome = "Welcome to the Cassio Operating System!\n";
    for (u32 i = 0; welcome[i] != '\0'; ++i) {
        terminal.putchar(welcome[i]);
    }

    while (true) {
        Message msg;
        i32 sender = IPC::receive(&msg);

        switch (msg.type) {
        case MessageType::VgaPutchar:
            terminal.putchar(static_cast<char>(msg.arg1));
            break;

        case MessageType::VgaWrite: {
            const char* data = reinterpret_cast<const char*>(&msg.arg1);
            for (u32 i = 0; i < 20 && data[i] != '\0'; ++i) {
                terminal.putchar(data[i]);
            }
            break;
        }

        case MessageType::VgaClear:
            terminal.clear();
            break;

        case MessageType::VgaSetCursor:
            terminal.setCursor(
                static_cast<u8>(msg.arg1),
                static_cast<u8>(msg.arg2));
            break;

        case MessageType::VgaGetCursor:
            break;

        default:
            break;
        }

        if (sender > 0) {
            Message reply = {};
            reply.arg1 = terminal.getCursorX();
            reply.arg2 = terminal.getCursorY();
            IPC::reply(static_cast<u32>(sender), &reply);
        }
    }
}
