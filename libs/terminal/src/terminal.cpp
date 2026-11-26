/**
 * terminal.cpp -- Terminal service client implementation
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#include <std/ipc.hpp>
#include <std/ns.hpp>
#include <std/terminal.hpp>

using namespace std;

terminal::Terminal::Terminal() : pid(0) {
    while (pid == 0) {
        pid = ns::lookup("terminal");
    }
}

void terminal::Terminal::putchar(char c) {
    ipc::Message msg = {};
    msg.type = ipc::MessageType::TerminalPutchar;
    msg.arg1 = static_cast<u8>(c);
    ipc::notify(pid, &msg);
}

void terminal::Terminal::write(str::StringView text) {
    for (u32 offset = 0; offset < text.length(); offset += WRITE_MAX) {
        u32 len = text.length() - offset;
        if (len > WRITE_MAX) {
            len = WRITE_MAX;
        }
        ipc::Message msg = {};
        msg.type = ipc::MessageType::TerminalWrite;
        msg.arg1 = len;
        ipc::notify(pid, &msg, text.data() + offset, len);
    }
}

void terminal::Terminal::clear() {
    ipc::Message msg = {};
    msg.type = ipc::MessageType::TerminalClear;
    ipc::notify(pid, &msg);
}

void terminal::Terminal::flush() {
    ipc::Message msg = {};
    msg.type = ipc::MessageType::TerminalFlush;
    ipc::notify(pid, &msg);
}

void terminal::Terminal::setCursor(u32 col, u32 row) {
    ipc::Message msg = {};
    msg.type = ipc::MessageType::TerminalSetCursor;
    msg.arg1 = col;
    msg.arg2 = row;
    ipc::send(pid, &msg);
}

void terminal::Terminal::getCursor(u32& col, u32& row) {
    ipc::Message msg = {};
    msg.type = ipc::MessageType::TerminalGetCursor;
    ipc::send(pid, &msg);
    col = msg.arg1;
    row = msg.arg2;
}
