/**
 * main.cpp -- Terminal service entry point
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 * Userspace graphics terminal service. Registers as "terminal"
 * with the nameserver and handles text output requests via IPC.
 * Renders characters to the display service using std::gfx.
 *
 */

#include <std/display.hpp>
#include <std/ipc.hpp>
#include <std/ns.hpp>
#include <std/types.hpp>

#include <terminal.hpp>

using namespace cassio;
using namespace std;

extern "C" void _start() {
    ns::registerName("terminal");

    display::Display display;
    display::DisplayInfo info = display.getInfo();

    Terminal terminal(display, info.width, info.height);

    const char* welcome = "Welcome to the Cassio Operating System!\n";
    for (u32 i = 0; welcome[i] != '\0'; ++i) {
        terminal.putchar(welcome[i]);
    }
    terminal.drawCursor();
    display.flush();

    while (true) {
        ipc::Message msg;
        char dataBuf[256];
        i32 sender = ipc::receive(&msg, dataBuf, sizeof(dataBuf));

        terminal.eraseCursor();

        switch (msg.type) {
        case ipc::MessageType::TerminalPutchar:
            terminal.putchar(static_cast<char>(msg.arg1));
            break;

        case ipc::MessageType::TerminalWrite: {
            u32 len = msg.arg1;
            if (len > sizeof(dataBuf))
                len = sizeof(dataBuf);
            for (u32 i = 0; i < len; ++i) {
                terminal.putchar(dataBuf[i]);
            }
            break;
        }

        case ipc::MessageType::TerminalClear:
            terminal.clear();
            break;

        case ipc::MessageType::TerminalSetCursor:
            terminal.setCursor(static_cast<u8>(msg.arg1), static_cast<u8>(msg.arg2));
            break;

        case ipc::MessageType::TerminalGetCursor:
            break;

        default:
            break;
        }

        terminal.drawCursor();
        display.flush();

        if (sender > 0) {
            ipc::Message reply = {};
            reply.arg1 = terminal.getCursorX();
            reply.arg2 = terminal.getCursorY();
            ipc::reply(static_cast<u32>(sender), &reply);
        }
    }
}
